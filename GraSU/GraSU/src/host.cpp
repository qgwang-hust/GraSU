#include <fstream>
#include <iostream>

#include <CL/cl_ext.h>

#include "config.h"
#include "pma_dynamic_graph.hpp"

const int max_event_num = 100;
int now_event_num = 0;

size_t node_size, static_edge_size, update_edge_size;

void read_dataset(const char *graphFilename, unsigned long *&static_edges, unsigned long *&update_edges);
void merge_data(pma_dynamic_graph &graph, unsigned int *data[4]);
bool check_result(unsigned long *data, size_t data_size, char *resultFilename, std::vector<unsigned int> &node_map);
void output(unsigned long *data, size_t data_size, std::vector<unsigned int> &node_map);

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

    cl::Kernel bin_search_kernel_1(program, "bin_search:{bin_search_1}");
    cl::Kernel bin_search_kernel_2(program, "bin_search:{bin_search_2}");
    cl::Kernel bin_search_kernel_3(program, "bin_search:{bin_search_3}");
    cl::Kernel bin_search_kernel_4(program, "bin_search:{bin_search_4}");

    cl::Kernel dispatch_kernel_1(program, "dispatch:{dispatch_1}");

    cl::Kernel process_cache_kernel_1(program, "process_cache:{process_cache_1}");
    cl::Kernel process_cache_kernel_2(program, "process_cache:{process_cache_2}");
    cl::Kernel process_ddr_kernel_1(program, "process_ddr:{process_ddr_1}");
    cl::Kernel process_ddr_kernel_2(program, "process_ddr:{process_ddr_2}");
    // cl::Kernel process_leaf_kernel_3(program, "process_leaf:{process_leaf_3}");
    // cl::Kernel process_leaf_kernel_4(program, "process_leaf:{process_leaf_4}");

    std::vector<cl::Event> events;
    events.resize(max_event_num);

    unsigned long *static_edges = NULL, *update_edges = NULL;
    unsigned long *sub_update_edges[4];
    size_t sub_update_edge_size[4];
    read_dataset(graphFilename, static_edges, update_edges);

    printf("Graph file is loaded.\n");

    pma_dynamic_graph graph(node_size, static_edges, static_edge_size, update_edges, update_edge_size);

    for (int i = 0; i < 4; i++) sub_update_edge_size[i] = update_edge_size / 4;
    for (size_t i = 0; i < (update_edge_size % 4); i++) sub_update_edge_size[i]++;
    cl_mem_ext_ptr_t update_edges_ptr[4];
    for (int i = 0; i < 4; i++) {
        update_edges_ptr[i].banks = XCL_MEM_DDR_BANK0 << i;
        update_edges_ptr[i].obj = update_edges_ptr[i].param = 0;
    }
    cl::Buffer update_edges_device_1(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * sub_update_edge_size[0], update_edges_ptr + 0);
    cl::Buffer update_edges_device_2(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * sub_update_edge_size[1], update_edges_ptr + 1);
    cl::Buffer update_edges_device_3(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * sub_update_edge_size[2], update_edges_ptr + 2);
    cl::Buffer update_edges_device_4(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * sub_update_edge_size[3], update_edges_ptr + 3);

    sub_update_edges[0] = (unsigned long *)cmdQueue.enqueueMapBuffer(update_edges_device_1, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * sub_update_edge_size[0]);
    sub_update_edges[1] = (unsigned long *)cmdQueue.enqueueMapBuffer(update_edges_device_2, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * sub_update_edge_size[1]);
    sub_update_edges[2] = (unsigned long *)cmdQueue.enqueueMapBuffer(update_edges_device_3, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * sub_update_edge_size[2]);
    sub_update_edges[3] = (unsigned long *)cmdQueue.enqueueMapBuffer(update_edges_device_4, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * sub_update_edge_size[3]);

    for (size_t i = 0; i < update_edge_size; i++) {
        sub_update_edges[i % 4][i / 4] = update_edges[i];
    }

    // prepare data
    unsigned int *data[4];  // 调整data数组的结构，我们认为data的前32位实际上内在的隐含在了row_offset数组中，因此我们将其从data中略去
    unsigned long *row_offset[4], *binary[4];
    size_t sub_data_size[2];
    size_t data_size = graph.data.size();
    size_t data_segment_size = data_size / SEGMENT_SIZE;
    size_t sub_data_segment_size[2];
    // 确定四个SLR上的data数据的大小
    for (int i = 0; i < 2; i++) sub_data_segment_size[i] = data_segment_size >> 1;
    for (size_t i = 0; i < (data_segment_size % 2); i++) sub_data_segment_size[i]++;
    for (int i = 0; i < 2; i++) {
        if (sub_data_segment_size[i] < MAX_CACHE_SEGMENT) {
            sub_data_segment_size[i] = MAX_CACHE_SEGMENT;
        }
    }
    for (int i = 0; i < 2; i++) sub_data_size[i] = sub_data_segment_size[i] * SEGMENT_SIZE;

    // 这里的数据这样构建，四块DDR上都包含数据，其中data_device_1和data_device_2上使用相同的数据，data_device_3和data_device_4上使用相同的数据
    // 两者的数据量分别为sub_data_size[0]和[1]
    cl_mem_ext_ptr_t data_ptr[4];
    for (int i = 0; i < 4; i++) {
        data_ptr[i].banks = XCL_MEM_DDR_BANK0 << i;
        data_ptr[i].obj = data_ptr[i].param = 0;
    }
    cl::Buffer data_device_1(context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned int) * sub_data_size[0], data_ptr + 0);
    cl::Buffer data_device_2(context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned int) * sub_data_size[0], data_ptr + 1);
    cl::Buffer data_device_3(context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned int) * sub_data_size[1], data_ptr + 2);
    cl::Buffer data_device_4(context, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned int) * sub_data_size[1], data_ptr + 3);
    data[0] = (unsigned int *)cmdQueue.enqueueMapBuffer(data_device_1, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned int) * sub_data_size[0]);
    data[1] = (unsigned int *)cmdQueue.enqueueMapBuffer(data_device_2, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned int) * sub_data_size[0]);
    data[2] = (unsigned int *)cmdQueue.enqueueMapBuffer(data_device_3, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned int) * sub_data_size[1]);
    data[3] = (unsigned int *)cmdQueue.enqueueMapBuffer(data_device_4, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned int) * sub_data_size[1]);
    for (size_t i = 0; i < data_size; i += SEGMENT_SIZE) {
        size_t segment_index = i / SEGMENT_SIZE;
        size_t sub_index = segment_index % 2;
        sub_index <<= 1;
        for (int j = 0; j < SEGMENT_SIZE; j++) {
            data[sub_index + 0][segment_index / 2 * SEGMENT_SIZE + j] = graph.data[i + j];
            data[sub_index + 1][segment_index / 2 * SEGMENT_SIZE + j] = graph.data[i + j];
            if (graph.data[i+j] & 0x8000000000000000UL) {
                data[sub_index + 0][segment_index / 2 * SEGMENT_SIZE + j] |= 0x80000000U;
                data[sub_index + 1][segment_index / 2 * SEGMENT_SIZE + j] |= 0x80000000U;
            }
        }
    }

    // prepare binary
    cl_mem_ext_ptr_t row_offset_ptr[4], binary_ptr[4];
    for (int i = 0; i < 4; i++) {
        row_offset_ptr[i].banks = XCL_MEM_DDR_BANK0 << i;
        binary_ptr[i].banks = XCL_MEM_DDR_BANK0 << i;
        row_offset_ptr[i].obj = row_offset_ptr[i].param = 0;
        binary_ptr[i].obj = binary_ptr[i].param = 0;
    }
    // printf("row_offset size is %ld bytes\n", sizeof(unsigned long) * graph.row_offset.size());
    // printf("binary size is %ld bytes\n", sizeof(unsigned long) * graph.binary_search.size());
    cl::Buffer row_offset_device_1(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.row_offset.size(), row_offset_ptr + 0);
    cl::Buffer row_offset_device_2(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.row_offset.size(), row_offset_ptr + 1);
    cl::Buffer row_offset_device_3(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.row_offset.size(), row_offset_ptr + 2);
    cl::Buffer row_offset_device_4(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.row_offset.size(), row_offset_ptr + 3);
    row_offset[0] = (unsigned long *)cmdQueue.enqueueMapBuffer(row_offset_device_1, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.row_offset.size());
    row_offset[1] = (unsigned long *)cmdQueue.enqueueMapBuffer(row_offset_device_2, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.row_offset.size());
    row_offset[2] = (unsigned long *)cmdQueue.enqueueMapBuffer(row_offset_device_3, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.row_offset.size());
    row_offset[3] = (unsigned long *)cmdQueue.enqueueMapBuffer(row_offset_device_4, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.row_offset.size());
    cl::Buffer binary_device_1(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.binary_search.size(), binary_ptr + 0);
    cl::Buffer binary_device_2(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.binary_search.size(), binary_ptr + 1);
    cl::Buffer binary_device_3(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.binary_search.size(), binary_ptr + 2);
    cl::Buffer binary_device_4(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX, sizeof(unsigned long) * graph.binary_search.size(), binary_ptr + 3);
    binary[0] = (unsigned long *)cmdQueue.enqueueMapBuffer(binary_device_1, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.binary_search.size());
    binary[1] = (unsigned long *)cmdQueue.enqueueMapBuffer(binary_device_2, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.binary_search.size());
    binary[2] = (unsigned long *)cmdQueue.enqueueMapBuffer(binary_device_3, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.binary_search.size());
    binary[3] = (unsigned long *)cmdQueue.enqueueMapBuffer(binary_device_4, CL_TRUE, CL_MAP_WRITE, 0, sizeof(unsigned long) * graph.binary_search.size());
    for (size_t i = 0; i < graph.row_offset.size() - 1; i++)
        for (int j = 0; j < 4; j++){
            row_offset[j][i] = graph.row_offset[i];
            row_offset[j][i] <<= 32;
            row_offset[j][i] += graph.row_offset[i + 1];
        }
    
    unsigned long max_row_offset_len = 0;
    for (size_t i = 1; i < graph.row_offset.size(); i++) {
        // if (graph.row_offset[i] - graph.row_offset[i - 1] > 1024) {
        //     printf("row_offset[%ld] == %ld, row_offset[%ld] == %ld\n", i - 1, graph.row_offset[i - 1], i, graph.row_offset[i]);
        // }
        max_row_offset_len = std::max(max_row_offset_len, graph.row_offset[i] - graph.row_offset[i - 1]);
    }
    printf("max is %ld\n", max_row_offset_len);

    for (size_t i = 0; i < graph.binary_search.size(); i++)
        for (int j = 0; j < 4; j++)
            binary[j][i] = graph.binary_search[i];

    printf("pre process finish\n");

    int task_event_num = now_event_num;
    cmdQueue.enqueueMigrateMemObjects({update_edges_device_1, data_device_1, row_offset_device_1, binary_device_1}, 0, NULL, &events[now_event_num++]);
    cmdQueue.enqueueMigrateMemObjects({update_edges_device_2, data_device_2, row_offset_device_2, binary_device_2}, 0, NULL, &events[now_event_num++]);
    cmdQueue.enqueueMigrateMemObjects({update_edges_device_3, data_device_3, row_offset_device_3, binary_device_3}, 0, NULL, &events[now_event_num++]);
    cmdQueue.enqueueMigrateMemObjects({update_edges_device_4, data_device_4, row_offset_device_4, binary_device_4}, 0, NULL, &events[now_event_num++]);
    for (int i = task_event_num; i < now_event_num; i++)
        events[i].wait();

    int arg_num = 0;
    bin_search_kernel_1.setArg(arg_num++, update_edges_device_1);
    for (int i = 0; i < 4; i++)
        bin_search_kernel_1.setArg(arg_num++, binary_device_1);
    for (int i = 0; i < 4; i++)
        bin_search_kernel_1.setArg(arg_num++, row_offset_device_1);
    bin_search_kernel_1.setArg(arg_num++, (unsigned int)sub_update_edge_size[0]);

    arg_num = 0;
    bin_search_kernel_2.setArg(arg_num++, update_edges_device_2);
    for (int i = 0; i < 4; i++)
        bin_search_kernel_2.setArg(arg_num++, binary_device_2);
    for (int i = 0; i < 4; i++)
        bin_search_kernel_2.setArg(arg_num++, row_offset_device_2);
    bin_search_kernel_2.setArg(arg_num++, (unsigned int)sub_update_edge_size[1]);

    arg_num = 0;
    bin_search_kernel_3.setArg(arg_num++, update_edges_device_3);
    for (int i = 0; i < 4; i++)
        bin_search_kernel_3.setArg(arg_num++, binary_device_3);
    for (int i = 0; i < 4; i++)
        bin_search_kernel_3.setArg(arg_num++, row_offset_device_3);
    bin_search_kernel_3.setArg(arg_num++, (unsigned int)sub_update_edge_size[2]);

    arg_num = 0;
    bin_search_kernel_4.setArg(arg_num++, update_edges_device_4);
    for (int i = 0; i < 4; i++)
        bin_search_kernel_4.setArg(arg_num++, binary_device_4);
    for (int i = 0; i < 4; i++)
        bin_search_kernel_4.setArg(arg_num++, row_offset_device_4);
    bin_search_kernel_4.setArg(arg_num++, (unsigned int)sub_update_edge_size[3]);

    dispatch_kernel_1.setArg(0, (unsigned int)update_edge_size);

    process_cache_kernel_1.setArg(0, data_device_1);

    process_ddr_kernel_1.setArg(0, data_device_2);
    process_ddr_kernel_1.setArg(1, data_device_2);
    process_ddr_kernel_1.setArg(2, data_device_2);
    process_ddr_kernel_1.setArg(3, data_device_2);

    process_cache_kernel_2.setArg(0, data_device_3);

    process_ddr_kernel_2.setArg(0, data_device_4);
    process_ddr_kernel_2.setArg(1, data_device_4);
    process_ddr_kernel_2.setArg(2, data_device_4);
    process_ddr_kernel_2.setArg(3, data_device_4);
    printf("kernel start running...\n");

    task_event_num = now_event_num;
    cmdQueue.enqueueTask(process_cache_kernel_1, NULL, &events[now_event_num++]);
    cmdQueue.enqueueTask(process_cache_kernel_2, NULL, &events[now_event_num++]);
    cmdQueue.enqueueTask(process_ddr_kernel_1, NULL, &events[now_event_num++]);
    cmdQueue.enqueueTask(process_ddr_kernel_2, NULL, &events[now_event_num++]);
    cmdQueue.enqueueTask(dispatch_kernel_1, NULL, &events[now_event_num++]);
    cmdQueue.enqueueTask(bin_search_kernel_1, NULL, &events[now_event_num++]);
    cmdQueue.enqueueTask(bin_search_kernel_2, NULL, &events[now_event_num++]);
    cmdQueue.enqueueTask(bin_search_kernel_3, NULL, &events[now_event_num++]);
    cmdQueue.enqueueTask(bin_search_kernel_4, NULL, &events[now_event_num++]);
    for (int i = task_event_num; i < now_event_num; i++)
        events[i].wait();

    printf("kernel finish\n");

    cl_ulong start[max_event_num];
    cl_ulong end[max_event_num];
    cl_ulong start_time = 0xFFFFFFFFFFFFFFFUL, end_time = 0;
    for (int i = task_event_num; i < now_event_num; i++) {
        start[i] = events[i].getProfilingInfo<CL_PROFILING_COMMAND_START>();
        end[i] = events[i].getProfilingInfo<CL_PROFILING_COMMAND_END>();
        start_time = std::min(start_time, start[i]);
        end_time = std::max(end_time, end[i]);
    }
    end_time -= start_time;
    printf("time is %.6lf ms\n", end_time / 1000000.0);
    printf("throughput is %.6lf M updates per second\n", 1000.0 * update_edge_size / end_time);

    task_event_num = now_event_num;
    cmdQueue.enqueueMigrateMemObjects({data_device_1, data_device_2, data_device_3, data_device_4}, CL_MIGRATE_MEM_OBJECT_HOST, NULL, &events[now_event_num++]);
    for (int i = task_event_num; i < now_event_num; i++)
        events[i].wait();

    merge_data(graph, data);
    bool check = check_result(graph.data.data(), graph.data.size(), resultFilename, graph.node_map);
    if (check) {
        fprintf(stderr, "check result passed\n");
    } else {
        fprintf(stderr, "check result failed\n");
    }

    if (OUTPUT_RESULT)
        output(graph.data.data(), graph.data.size(), graph.node_map);

    task_event_num = now_event_num;
    cmdQueue.enqueueUnmapMemObject(data_device_1, data[0], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(data_device_2, data[1], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(data_device_3, data[2], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(data_device_4, data[3], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(update_edges_device_1, sub_update_edges[0], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(update_edges_device_2, sub_update_edges[1], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(update_edges_device_3, sub_update_edges[2], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(update_edges_device_4, sub_update_edges[3], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(row_offset_device_1, row_offset[0], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(row_offset_device_2, row_offset[1], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(row_offset_device_3, row_offset[2], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(row_offset_device_4, row_offset[3], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(binary_device_1, binary[0], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(binary_device_2, binary[1], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(binary_device_3, binary[2], NULL, &events[now_event_num++]);
    cmdQueue.enqueueUnmapMemObject(binary_device_4, binary[3], NULL, &events[now_event_num++]);
    for (int i = task_event_num; i < now_event_num; i++)
        events[i].wait();

    free(static_edges);
    free(update_edges);

    cmdQueue.finish();
    return 0;
}

bool check_result(unsigned long *data, size_t data_size, char *resultFilename, std::vector<unsigned int> &node_map) {
    FILE *result = fopen(resultFilename, "r");
    bool check = true;
    unsigned long start_node, end_node;
    std::vector<unsigned long> result_edges;
    while (fscanf(result, "%lx -> %lx", &start_node, &end_node) != EOF) {
        start_node = node_map[start_node];
        end_node = node_map[end_node];
        result_edges.push_back((start_node << 32) + end_node);
    }
    fclose(result);
    std::sort(result_edges.begin(), result_edges.end());
    auto result_edge_i = result_edges.begin();
    for (size_t i = 0; i < data_size; i++) {
        if (data[i] != EMPTY) {
            if (data[i] != *result_edge_i) {
                printf("true: %lx\n", *result_edge_i);
                printf("got : %lx\n", data[i]);
                check = false;
                break;
            }
            result_edge_i++;
        }
    }
    if (result_edge_i != result_edges.end()) {
        check = false;
    }
    
    return check;
}

void output(unsigned long *data, size_t data_size, std::vector<unsigned int> &node_map) {
    std::vector<unsigned int> trans_node_map;
    trans_node_map.resize(node_map.size());
    for (size_t i = 0; i < node_map.size(); i++)
        trans_node_map[node_map[i]] = i;

    std::vector<unsigned long> output_edges;
    unsigned long begin_node, end_node;
    for (size_t i = 0; i < data_size; i++) {
        if (data[i] != EMPTY) {
            begin_node = data[i] >> 32;
            end_node = data[i] & 0xFFFFFFFFUL;
            begin_node = trans_node_map[begin_node];
            end_node = trans_node_map[end_node];
            output_edges.push_back((begin_node << 32) + end_node);
        }
    }
    std::sort(output_edges.begin(), output_edges.end());
    printf("there are %ld segments\n", data_size / SEGMENT_SIZE);
    for (size_t i = 0; i < output_edges.size(); i++) {
        printf("%x -> %x\n", (unsigned int)(output_edges[i] >> 32), (unsigned int)output_edges[i]);
    }
}

bool cmp(std::pair<unsigned int, double> a, std::pair<unsigned int, double> b) {
    return a.second > b.second;
}

void read_dataset(const char *graphFilename, unsigned long *&static_edges, unsigned long *&update_edges) {
    FILE *fp = fopen(graphFilename, "r");
    fscanf(fp, "%lu %lu %lu", &node_size, &static_edge_size, &update_edge_size);
    printf("node_num: %lu\nstatic_edge_num: %lu\nupdate_edge_num: %lu\n", node_size, static_edge_size, update_edge_size);

    static_edges = (unsigned long *)malloc(sizeof(unsigned long) * static_edge_size);
    for (size_t i = 0; i < static_edge_size; i++) {
        int begin_node, end_node;
        fscanf(fp, "%d %d", &begin_node, &end_node);
        static_edges[i] = ((unsigned long)(begin_node) << 32) + end_node;
    }
    
    update_edges = (unsigned long *)malloc(sizeof(unsigned long) * update_edge_size);
    printf("update_edge_size is %ld\n", update_edge_size);
    for (size_t i = 0; i < update_edge_size; i++) {
        int begin_node, end_node, type;
        fscanf(fp, "%d %d %d", &begin_node, &end_node, &type);
        update_edges[i] = ((unsigned long)(begin_node) << 32) + end_node;
        if (type == 0) update_edges[i] += EMPTY;    // delete
    }
    fclose(fp);
    printf("read graph file finish\n");
}

void merge_data(pma_dynamic_graph &graph, unsigned int *data[4]) {
    unsigned long source_node = 0;
    size_t i = 0;
    for (; i < (MAX_CACHE_SEGMENT * SEGMENT_SIZE * 2) && i < graph.data.size(); i += SEGMENT_SIZE) {
        size_t segment_index = i / SEGMENT_SIZE;
        while (graph.row_offset[source_node+1] <= i)
            source_node++;
        size_t sub_index = segment_index % 2;
        sub_index <<= 1;
        for (int j = 0; j < SEGMENT_SIZE; j++) {
            if (data[sub_index][segment_index / 2 * SEGMENT_SIZE + j] & 0x80000000U) {
                graph.data[i + j] = EMPTY;
            } else {
                graph.data[i + j] = (source_node << 32) + (unsigned long)data[sub_index][segment_index / 2 * SEGMENT_SIZE + j];
            }
        }
    }
    for (; i < graph.data.size(); i += SEGMENT_SIZE) {
        size_t segment_index = i / SEGMENT_SIZE;
        while (graph.row_offset[source_node+1] <= i)
            source_node++;
        size_t sub_index = segment_index % 2;
        sub_index = (sub_index << 1) + 1;
        for (int j = 0; j < SEGMENT_SIZE; j++) {
            if (data[sub_index][segment_index / 2 * SEGMENT_SIZE + j] & 0x80000000U) {
                graph.data[i + j] = EMPTY;
            } else {
                graph.data[i + j] = (source_node << 32) + (unsigned long)data[sub_index][segment_index / 2 * SEGMENT_SIZE + j];
            }
        }
    }
}
