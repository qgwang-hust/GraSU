
#include "kernel_config.h"

extern "C" {
void read_edges(
    const unsigned long *edges,
    const unsigned int edge_size,
    hls::stream<pipe_type> &edge_stream_0,
    hls::stream<pipe_type> &edge_stream_1,
    hls::stream<pipe_type> &edge_stream_2,
    hls::stream<pipe_type> &edge_stream_3) {
#pragma HLS INTERFACE m_axi port = edges offset = slave bundle = gmem0
#pragma HLS INTERFACE s_axilite port = edges bundle = control
#pragma HLS INTERFACE s_axilite port = edge_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS INTERFACE axis port = edge_stream_0
#pragma HLS INTERFACE axis port = edge_stream_1
#pragma HLS INTERFACE axis port = edge_stream_2
#pragma HLS INTERFACE axis port = edge_stream_3

#pragma HLS STREAM variable=edge_stream_0 depth=64
#pragma HLS STREAM variable=edge_stream_1 depth=64
#pragma HLS STREAM variable=edge_stream_2 depth=64
#pragma HLS STREAM variable=edge_stream_3 depth=64

    pipe_type pipe_data;
    for (int i = 0; i < edge_size; i++) {
        #pragma hls pipeline II=1 rewind
        pipe_data.data = edges[i];
        switch (i & (4 - 1)) {
        case 0:
            edge_stream_0.write(pipe_data);
            break;
        case 1:
            edge_stream_1.write(pipe_data);
            break;
        case 2:
            edge_stream_2.write(pipe_data);
            break;
        case 3:
            edge_stream_3.write(pipe_data);
            break;
        }
    }
}
}
