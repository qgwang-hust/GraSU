#include "kernel_config.h"

extern "C" {
void bin_search(
    const unsigned long *binary,
    const unsigned long *row_offset,
    const unsigned int size,
    hls::stream<pipe_type> &edge_stream,
    hls::stream<pipe_type_96> &segment_head_stream) {
#pragma HLS INTERFACE m_axi port = binary offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = row_offset offset = slave bundle = gmem1
#pragma HLS INTERFACE s_axilite port = binary bundle = control
#pragma HLS INTERFACE s_axilite port = row_offset bundle = control
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS INTERFACE axis port = edge_stream
#pragma HLS INTERFACE axis port = segment_head_stream

#pragma HLS STREAM variable=edge_stream depth=64
#pragma HLS STREAM variable=segment_head_stream depth=64

    unsigned long edge;
    ap_uint<96> res;
    pipe_type_96 pipe_data_96;
    printf("bin_search start\n");
    for (int ii = 0; ii < size; ii++) {
        edge = edge_stream.read().data.to_ulong();
        res = edge;
        res <<= 32;

        edge &= (~EMPTY);

        unsigned int node_begin = edge >> 32;
        unsigned int segment_head_begin, segment_head_end, segment_head_mid;
        unsigned int segment_left_mid, segment_right_mid;
        unsigned int left_left, left_right, right_left, right_right;
        unsigned int offset[2];
        unsigned long mid_data[7];
    get_offset_loop:
        for (int i = 0; i < 2; i++) {
            #pragma hls pipeline
            offset[i] = row_offset[node_begin + i] >> 4;
        }
        segment_head_begin = offset[0];
        segment_head_end = offset[1];
        segment_head_mid = ((segment_head_begin + segment_head_end) >> 1);
    binary_search_loop:
        while (segment_head_begin != segment_head_mid) {
            segment_left_mid = ((segment_head_begin + segment_head_mid) >> 1);
            segment_right_mid = ((segment_head_mid + segment_head_end) >> 1);
            left_left = ((segment_head_begin + segment_left_mid) >> 1);
            left_right = ((segment_left_mid + segment_head_mid) >> 1);
            right_left = ((segment_head_mid + segment_right_mid) >> 1);
            right_right = ((segment_right_mid + segment_head_end) >> 1);

            mid_data[0] = binary[left_left];
            mid_data[1] = binary[segment_left_mid];
            mid_data[2] = binary[left_right];
            mid_data[3] = binary[segment_head_mid];
            mid_data[4] = binary[right_left];
            mid_data[5] = binary[segment_right_mid];
            mid_data[6] = binary[right_right];

            if (mid_data[3] <= edge) {
                if (mid_data[5] <= edge) {
                    if (mid_data[6] <= edge) {
                        segment_head_begin = right_right;
                    } else {
                        segment_head_begin = segment_right_mid;
                        segment_head_end = right_right;
                    }
                } else {
                    if (mid_data[4] <= edge) {
                        segment_head_begin = right_left;
                        segment_head_end = segment_right_mid;
                    } else {
                        segment_head_begin = segment_head_mid;
                        segment_head_end = right_left;
                    }
                }
            } else {
                if (mid_data[1] <= edge) {
                    if (mid_data[2] <= edge) {
                        segment_head_begin = left_right;
                        segment_head_end = segment_head_mid;
                    } else {
                        segment_head_begin = segment_left_mid;
                        segment_head_end = left_right;
                    }
                } else {
                    if (mid_data[0] <= edge) {
                        segment_head_begin = left_left;
                        segment_head_end = segment_left_mid;
                    } else {
                        segment_head_end = left_left;
                    }
                }
            }
            segment_head_mid = ((segment_head_begin + segment_head_end) >> 1);
        }
        pipe_data_96.data = res + (segment_head_begin << 4);
        segment_head_stream.write(pipe_data_96);
    }
    printf("bin_search finish\n");
}
}