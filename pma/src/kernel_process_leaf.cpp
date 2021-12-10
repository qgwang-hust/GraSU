#include "kernel_config.h"

extern "C" {
void process_leaf(
    unsigned long *data,
    hls::stream<pipe_type_96> &left_location_stream) {
#pragma HLS INTERFACE m_axi port = data offset = slave bundle = gmem2
#pragma HLS INTERFACE s_axilite port = data bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS INTERFACE axis port = left_location_stream
#pragma HLS STREAM variable=left_location_stream depth=64

    unsigned long tmpData[16];
    // #pragma HLS ARRAY_RESHAPE variable=tmpData complete dim=1
    unsigned long cur_data, edge;
    unsigned int segment_count, segment_head;
    unsigned int type;

    ap_uint<96> info;
    printf("process_leaf start\n");

    while (true) {
        info = left_location_stream.read().data;
        if (info.to_ulong() == END_EDGE) {
            printf("process_leaf finish\n");
            break;
        }

        edge = (info >> 32).to_ulong();
        segment_head = info.to_int();

        if ((edge & EMPTY) == EMPTY)
            type = 0; // delete
        else
            type = 1; // insert
        edge &= (~EMPTY);
        segment_count = 0;

    read_in_data_loop:
        for (int i = 0; i < 16; i++) {
            #pragma hls pipeline
            tmpData[i] = data[segment_head + i];
        }

        if (type == 1) {
        insert_find_loop:
            for (int i = 0; i < 16; i++) {
                #pragma hls unroll
                if (tmpData[i] < edge) {
                    segment_count++;
                }
            }
        insert_loop:
            for (int i = 0; i < 16; i++) {
                #pragma hls pipeline
                if (i >= segment_count) {
                    cur_data = tmpData[i];
                    tmpData[i] = edge;
                    edge = cur_data;
                }
            }
        } else {
        delete_find_loop:
            for (int i = 0; i < 16; i++) {
                #pragma hls unroll
                if (tmpData[i] == edge) {
                    segment_count = i;
                }
            }
        delete_loop:
            for (int i = segment_count; i < 16 - 1; i++) {
                #pragma hls pipeline
                tmpData[i] = tmpData[i + 1];
            }
            tmpData[15] = EMPTY;
        }

    tmpData_to_data:
        for (int i = 0; i < 16; i++) {
            #pragma hls pipeline
            data[segment_head + i] = tmpData[i];
        }
    }
}
}
