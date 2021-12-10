#include "kernel_config.h"

extern "C" {
void dispatch(
    const int size,
    hls::stream<pipe_type_96> &segment_head_stream_0,
    hls::stream<pipe_type_96> &segment_head_stream_1,
    hls::stream<pipe_type_96> &segment_head_stream_2,
    hls::stream<pipe_type_96> &segment_head_stream_3,
    hls::stream<pipe_type_96> &left_location_stream_0,
    hls::stream<pipe_type_96> &left_location_stream_1,
    hls::stream<pipe_type_96> &left_location_stream_2,
    hls::stream<pipe_type_96> &left_location_stream_3) {
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS INTERFACE axis port = segment_head_stream_0
#pragma HLS INTERFACE axis port = segment_head_stream_1
#pragma HLS INTERFACE axis port = segment_head_stream_2
#pragma HLS INTERFACE axis port = segment_head_stream_3
#pragma HLS INTERFACE axis port = left_location_stream_0
#pragma HLS INTERFACE axis port = left_location_stream_1
#pragma HLS INTERFACE axis port = left_location_stream_2
#pragma HLS INTERFACE axis port = left_location_stream_3

#pragma HLS STREAM variable=segment_head_stream_0 depth=64
#pragma HLS STREAM variable=segment_head_stream_1 depth=64
#pragma HLS STREAM variable=segment_head_stream_2 depth=64
#pragma HLS STREAM variable=segment_head_stream_3 depth=64
#pragma HLS STREAM variable=left_location_stream_0 depth=64
#pragma HLS STREAM variable=left_location_stream_1 depth=64
#pragma HLS STREAM variable=left_location_stream_2 depth=64
#pragma HLS STREAM variable=left_location_stream_3 depth=64

    int loc;
    pipe_type_96 pipe_data_96[4];
dispatch_update:
    for (int i = 0; i < (size >> 2); i++) {
        #pragma hls pipeline II=1 rewind
        pipe_data_96[0].data = segment_head_stream_0.read().data;
        pipe_data_96[1].data = segment_head_stream_1.read().data;
        pipe_data_96[2].data = segment_head_stream_2.read().data;
        pipe_data_96[3].data = segment_head_stream_3.read().data;

        for (int j = 0; j < 4; j++) {
            #pragma hls unroll
            loc = ((pipe_data_96[j].data >> 4) & (4 - 1)).to_uint();
            switch (loc) {
            case 0:
                left_location_stream_0.write(pipe_data_96[j]);
                break;
            case 1:
                left_location_stream_1.write(pipe_data_96[j]);
                break;
            case 2:
                left_location_stream_2.write(pipe_data_96[j]);
                break;
            case 3:
                left_location_stream_3.write(pipe_data_96[j]);
                break;
            }
        }
        // switch (i & (4 - 1)) {
        // case 0:
        //     pipe_data_96.data = segment_head_stream_0.read().data;
        //     break;
        // case 1:
        //     pipe_data_96.data = segment_head_stream_1.read().data;
        //     break;
        // case 2:
        //     pipe_data_96.data = segment_head_stream_2.read().data;
        //     break;
        // case 3:
        //     pipe_data_96.data = segment_head_stream_3.read().data;
        //     break;
        // }
        // loc = ((pipe_data_96.data >> 4) & (4 - 1)).to_uint();
        // switch (loc) {
        // case 0:
        //     left_location_stream_0.write(pipe_data_96);
        //     break;
        // case 1:
        //     left_location_stream_1.write(pipe_data_96);
        //     break;
        // case 2:
        //     left_location_stream_2.write(pipe_data_96);
        //     break;
        // case 3:
        //     left_location_stream_3.write(pipe_data_96);
        //     break;
        // }
    }
    pipe_data_96[0].data = END_EDGE;
    left_location_stream_0.write(pipe_data_96[0]);
    left_location_stream_1.write(pipe_data_96[0]);
    left_location_stream_2.write(pipe_data_96[0]);
    left_location_stream_3.write(pipe_data_96[0]);
    printf("dispatch finish\n");
}
}