#include <fstream>
#include <iostream>

#include <CL/cl_ext.h>

#include "config.h"
#include "pma_dynamic_graph.hpp"

const int max_event_num = 20;
int now_event_num = 0;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <xclbin> <graph_file> <result_file>" << std::endl;
        return EXIT_FAILURE;
    }

    char *xclbinFilename = argv[1];
    char *graphFilename = argv[2];
    char *resultFilename = argv[3];

    std::vector<cl::Platform> platforms;
    std::vector<cl::Device> devices;
    cl::Device device;
    bool found_device = false;

    cl::Platform::get(&platforms);
    for (size_t i = 0; i < platforms.size() && !found_device; i++) {
        cl::Platform platform = platforms[i];
        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>();
        if (platformName == "Xilinx") {
            devices.clear();
            platform.getDevices(CL_DEVICE_TYPE_ACCELERATOR, &devices);
            if (devices.size()) {
                device = devices[0];
                found_device = true;
            }
        }
    }

    if (!found_device) {
        printf("Error: Unable to find device\n");
        return EXIT_FAILURE;
    }

    cl::Context context(device);
    cl::CommandQueue cmdQueue(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    printf("Loading '%s'\n", xclbinFilename);
    std::ifstream binFile(xclbinFilename, std::ifstream::binary);
    binFile.seekg(0, binFile.end);
    unsigned int binFileSize = binFile.tellg();
    binFile.seekg(0, binFile.beg);
    char *binBuf = new char[binFileSize];
    binFile.read(binBuf, binFileSize);

    cl::Program::Binaries bins;
    bins.push_back({binBuf, binFileSize});
    devices.resize(1);
    cl::Program program(context, devices, bins);
    cl::Kernel read_edges_kernel(program, "read_edges");
    cl::Kernel bin_search_kernel(program, "bin_search");
    cl::Kernel dispatch_kernel(program, "dispatch");
    cl::Kernel process_leaf_kernel(program, "process_leaf");

    std::vector<cl::Event> events;
    events.resize(max_event_num);

    FILE *fp = fopen(graphFilename, "r");
    size_t node_size, static_edge_size, update_edge_size;
    if (!fp) {
        printf("Error: Unable to read data file\n");
        return EXIT_FAILURE;
    }
    fscanf(fp, "%lu %lu %lu", &node_size, &static_edge_size, &update_edge_size);
    printf("node_num: %lu\nstatic_edge_num: %lu\nupdate_edge_num: %lu\n", node_size, static_edge_size, update_edge_size);

    unsigned long *static_edges = (unsigned long *)malloc(sizeof(unsigned long) * static_edge_size);
    for (size_t i = 0; i < static_edge_size; i++) {
        int begin_node, end_node;
        fscanf(fp, "%d %d", &begin_node, &end_node);
        static_edges[i] = ((unsigned long)(begin_node) << 32) + end_node;
    }

    cl_mem_ext_ptr_t edges_ptr;
    edges_ptr.flags = XCL_MEM_DDR_BANK0;
    edges_ptr.obj = edges_ptr.param = 0;
    cl::Buffer edges_device(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * update_edge_size, &edges_ptr);
    unsigned long *edges = (unsigned long *)cmdQueue.enqueueMapBuffer(edges_device, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * update_edge_size);
    for (size_t i = 0; i < update_edge_size; i++) {
        int begin_node, end_node, type;
        fscanf(fp, "%d %d %d", &begin_node, &end_node, &type);
        edges[i] = ((unsigned long)(begin_node) << 32) + end_node;
        if (type == 0) edges[i] += 0x8000000000000000UL;    // delete
    }

    printf("Graph file is loaded.\n");
    fclose(fp);

    pma_dynamic_graph graph(node_size, static_edges, static_edge_size, edges, update_edge_size);

    printf("pre process finish\n");

    cl_mem_ext_ptr_t data_ptr, binary_ptr, row_offset_ptr;
    data_ptr.flags = XCL_MEM_DDR_BANK2;
    binary_ptr.flags = row_offset_ptr.flags = XCL_MEM_DDR_BANK1;
    data_ptr.obj = data_ptr.param = 0;
    binary_ptr.obj = binary_ptr.param = 0;
    row_offset_ptr.obj = row_offset_ptr.param = 0;
    cl::Buffer data_device(context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.data.size(), & data_ptr);
    cl::Buffer binary_search_device(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.binary_search.size(), &binary_ptr);
    cl::Buffer row_offset_device(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.row_offset.size(), &row_offset_ptr);

    unsigned long *data = (unsigned long *)cmdQueue.enqueueMapBuffer(data_device, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.data.size());
    for (size_t i = 0; i < graph.data.size(); i++) {
        data[i] = graph.data[i];
    }

    unsigned long *row_offset_device_local = (unsigned long *)cmdQueue.enqueueMapBuffer(row_offset_device, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.row_offset.size());
    for (size_t i = 0; i < graph.row_offset.size(); i++) {
        row_offset_device_local[i] = graph.row_offset[i];
    }

    unsigned long *binary_search_local = (unsigned long *)cmdQueue.enqueueMapBuffer(binary_search_device, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.binary_search.size());
    for (size_t i = 0; i < graph.binary_search.size(); i++) {
        binary_search_local[i] = graph.binary_search[i];
    }

    cmdQueue.enqueueMigrateMemObjects({edges_device, data_device, row_offset_device, binary_search_device}, 0, NULL, &events[now_event_num]);
    events[now_event_num].wait();
    now_event_num++;

    // update_edge_kernel.setArg(0, data_device);
    // update_edge_kernel.setArg(1, edges_device);
    // update_edge_kernel.setArg(2, row_offset_device);
    // update_edge_kernel.setArg(3, binary_search_device);
    // update_edge_kernel.setArg(4, ((unsigned int)update_edge_size >> 4) << 4);
    // update_edge_kernel.setArg(5, (unsigned int)graph.binary_search.size());
    update_edge_size = (update_edge_size >> 4) << 4;
    read_edges_kernel.setArg(0, edges_device);
    read_edges_kernel.setArg(1, ((unsigned int)update_edge_size >> 2) << 2);

    bin_search_kernel.setArg(0, binary_search_device);
    bin_search_kernel.setArg(1, row_offset_device);
    bin_search_kernel.setArg(2,  (unsigned int)update_edge_size >> 2);

    dispatch_kernel.setArg(0, ((unsigned int)update_edge_size >> 2) << 2);

    process_leaf_kernel.setArg(0, data_device);

    printf("kernel start running...\n");

    cmdQueue.enqueueTask(read_edges_kernel, NULL, &events[now_event_num]);
    cmdQueue.enqueueTask(bin_search_kernel, NULL, &events[now_event_num + 1]);
    cmdQueue.enqueueTask(bin_search_kernel, NULL, &events[now_event_num + 2]);
    cmdQueue.enqueueTask(bin_search_kernel, NULL, &events[now_event_num + 3]);
    cmdQueue.enqueueTask(bin_search_kernel, NULL, &events[now_event_num + 4]);
    cmdQueue.enqueueTask(dispatch_kernel, NULL, &events[now_event_num + 5]);
    cmdQueue.enqueueTask(process_leaf_kernel, NULL, &events[now_event_num + 6]);
    cmdQueue.enqueueTask(process_leaf_kernel, NULL, &events[now_event_num + 7]);
    cmdQueue.enqueueTask(process_leaf_kernel, NULL, &events[now_event_num + 8]);
    cmdQueue.enqueueTask(process_leaf_kernel, NULL, &events[now_event_num + 9]);
    events[now_event_num].wait();
    events[now_event_num + 1].wait();
    events[now_event_num + 2].wait();
    events[now_event_num + 3].wait();
    events[now_event_num + 4].wait();
    events[now_event_num + 5].wait();
    events[now_event_num + 6].wait();
    events[now_event_num + 7].wait();
    events[now_event_num + 8].wait();
    events[now_event_num + 9].wait();
    int task_event_num = now_event_num;
    now_event_num += 10;

    printf("kernel finish\n");

    cl_ulong start[10];
    cl_ulong end[10];
    cl_ulong start_time = 0xFFFFFFFFFFFFFFFUL, end_time = 0;
    for (int i = task_event_num; i < task_event_num + 10; i++) {
        start[i] = events[i].getProfilingInfo<CL_PROFILING_COMMAND_START>();
        end[i] = events[i].getProfilingInfo<CL_PROFILING_COMMAND_END>();
        start_time = std::min(start_time, start[i]);
        end_time = std::max(end_time, end[i]);
    }
    end_time -= start_time;
    printf("time is %.2lf ms\n", end_time / 1000000.0);
    printf("throughput is %.2lf M update per second\n", 1000.0 * update_edge_size / end_time);

    cmdQueue.enqueueMigrateMemObjects({data_device, row_offset_device}, CL_MIGRATE_MEM_OBJECT_HOST, NULL, &events[now_event_num]);
    events[now_event_num].wait();
    now_event_num++;

    FILE *result = fopen(resultFilename, "r");
    unsigned long start_node, end_node;
    bool check = true;
    for (size_t i = 0; i < graph.data.size(); i++) {
        if (data[i] != 0x8000000000000000UL) {
            // printf("%lx\n", data[i]);
            fscanf(result, "%lx -> %lx", &start_node, &end_node);
            // printf("%x -> %x\n", start_node, end_node);
            if (start_node != (unsigned int)(data[i] >> 32) || end_node != (unsigned int)(data[i])) {
                printf("true: %lx -> %lx\n", start_node, end_node);
                printf("got : %lu %x -> %x\n", i, (unsigned int)(data[i] >> 32), (unsigned int)(data[i]));
                check = false;
            }
        }
    }
    fclose(result);
    if (check) {
        printf("check result passed\n");
    } else {
        printf("check result failed\n");
    }

//    printf("data_size is %lu\n", graph.data.size());
//    for (size_t i = 0; i < graph.data.size(); i++) {
//        if (data[i] != 0x8000000000000000UL) {
//            printf("%x -> %x\n", (unsigned int)(data[i] >> 32), (unsigned int)(data[i]));
//        }
//    }

    cmdQueue.enqueueUnmapMemObject(data_device, data, NULL, &events[now_event_num]);
    cmdQueue.enqueueUnmapMemObject(edges_device, edges, NULL,  &events[now_event_num + 1]);
    cmdQueue.enqueueUnmapMemObject(row_offset_device, row_offset_device_local, NULL, &events[now_event_num + 2]);
    cmdQueue.enqueueUnmapMemObject(binary_search_device, binary_search_local, NULL, &events[now_event_num + 3]);
    for (int i = 0; i < 4; i++) {
        events[i + now_event_num].wait();
    }
    now_event_num += 4;
    cmdQueue.finish();
    return 0;
}
