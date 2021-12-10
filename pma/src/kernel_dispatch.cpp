#include "kernel_config.h"

extern "C" {
void dispatch(
    const int size,
    hls::stream<pipe_type_96> &segment_head_stream_1,
    hls::stream<pipe_type_96> &segment_head_stream_2,
    hls::stream<pipe_type_96> &segment_head_stream_3,
    hls::stream<pipe_type_96> &segment_head_stream_4,

    hls::stream<pipe_type_96> &dispatch_to_process_cache_1,
    hls::stream<pipe_type_96> &dispatch_to_process_cache_2,

    hls::stream<pipe_type_96> &dispatch_to_process_ddr_1,
    hls::stream<pipe_type_96> &dispatch_to_process_ddr_2) {
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS INTERFACE axis port = segment_head_stream_1
#pragma HLS INTERFACE axis port = segment_head_stream_2
#pragma HLS INTERFACE axis port = segment_head_stream_3
#pragma HLS INTERFACE axis port = segment_head_stream_4

#pragma HLS INTERFACE axis port = dispatch_to_process_cache_1
#pragma HLS INTERFACE axis port = dispatch_to_process_cache_2

#pragma HLS INTERFACE axis port = dispatch_to_process_ddr_1
#pragma HLS INTERFACE axis port = dispatch_to_process_ddr_2

    ap_uint<1> loc;
    ap_uint<32> index;
    pipe_type_96 pipe_data_96;

    for (size_t i = 0; i < size; i++) {
        switch (i & 3) {
        case 0:
            pipe_data_96 = segment_head_stream_1.read();
            break;
        case 1:
            pipe_data_96 = segment_head_stream_2.read();
            break;
        case 2:
            pipe_data_96 = segment_head_stream_3.read();
            break;
        case 3:
            pipe_data_96 = segment_head_stream_4.read();
            break;
        }
        loc = pipe_data_96.data[4];
        index = pipe_data_96.data.range(31, 5);
        if (index < MAX_CACHE_SEGMENT) {
            if (loc == 0) {
                dispatch_to_process_cache_1.write(pipe_data_96);
            } else {
                dispatch_to_process_cache_2.write(pipe_data_96);
            }
        } else {
            if (loc == 0) {
                dispatch_to_process_ddr_1.write(pipe_data_96);
            } else {
                dispatch_to_process_ddr_2.write(pipe_data_96);
            }
        }
    }
    pipe_data_96.data = END_EDGE;
    dispatch_to_process_cache_1.write(pipe_data_96);
    dispatch_to_process_cache_2.write(pipe_data_96);
    dispatch_to_process_ddr_1.write(pipe_data_96);
    dispatch_to_process_ddr_2.write(pipe_data_96);
    printf("dispatch finish\n");
}
}
