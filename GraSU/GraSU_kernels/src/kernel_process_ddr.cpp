#include "kernel_config.h"

#define UPDATE_STREAM_DDR_NUM 16
#define UPDATE_STREAM_DDR_NUM_LOG 4

void dispatch_update(
    hls::stream<pipe_type_96> &update_stream,
    hls::stream<ap_uint<96> > update_stream_ddr_0[UPDATE_STREAM_DDR_NUM],
    hls::stream<ap_uint<96> > update_stream_ddr_1[UPDATE_STREAM_DDR_NUM]) {

    ap_uint<96> data;
    dispatch_loop:
    while (true) {
        data = update_stream.read().data;
        if (data == END_EDGE)
            break;
        if (data[5] == 0) {
            update_stream_ddr_0[data.range(5 + UPDATE_STREAM_DDR_NUM_LOG, 6)].write(data);
        } else {
            update_stream_ddr_1[data.range(5 + UPDATE_STREAM_DDR_NUM_LOG, 6)].write(data);
        }
    }
    for (int i = 0; i < UPDATE_STREAM_DDR_NUM; i++) {
        #pragma HLS UNROLL
        update_stream_ddr_0[i].write(END_EDGE);
        update_stream_ddr_1[i].write(END_EDGE);
    }
}

void process(
    const ap_uint<512> *pma_in,
    ap_uint<512> *pma_out,
    hls::stream<ap_uint<96> > update_stream_ddr[UPDATE_STREAM_DDR_NUM]) {

    ap_uint<512> data;
    ap_uint<512> res_data;
    ap_uint<96> info;
    ap_uint<32> local_edges[16];
    ap_uint<32> update_edge;
    ap_uint<32> index;
    ap_uint<10> segment_count;
    ap_uint<4> cmp_res[16];
    ap_uint<1> delete_op;
    ap_uint<5> finish = 0;

    process_ddr_loop:
    while (true) {
        process_ddr_sub_loop:
        for (int i = 0; i < UPDATE_STREAM_DDR_NUM; i++) {
            #pragma HLS PIPELINE II=1
            if (update_stream_ddr[i].read_nb(info)) {
                if (info == END_EDGE) {
                    finish++;
                } else {
                    index = info.range(31, 5);
                    update_edge = info.range(63, 32);
                    delete_op = info[95];
                    segment_count = 0;

                    data = pma_in[index];

                    for (int i = 0; i < 16; i++) {
                        #pragma HLS UNROLL
                        local_edges[i] = data.range((i << 5) + 31, i << 5);
                        if (local_edges[i] < update_edge) cmp_res[i] = 1;
                        else cmp_res[i] = 0;
                    }

                    for (int i = 0; i < 16; i++) {
                        #pragma HLS UNROLL
                        segment_count += cmp_res[i];
                    }

                    segment_count <<= 5;
                    res_data = data.range(511, segment_count);

                    if (delete_op) {
                        res_data = res_data.range(511, 32);
                        if (segment_count != 0) {
                            res_data <<= segment_count;
                            res_data |= data.range(segment_count - 1, 0);
                        }
                        res_data.set_bit(511, true);
                    } else {
                        res_data <<= 32;
                        res_data |= update_edge;
                        if (segment_count != 0) {
                            res_data <<= segment_count;
                            res_data |= data.range(segment_count - 1, 0);
                        }
                    }

                    pma_out[index] = res_data;
                }
            }
        }
        if (finish == UPDATE_STREAM_DDR_NUM)
            break;
    }
}

extern "C" {
void process_ddr(
    const ap_uint<512> *pma_in0_ddr,
    const ap_uint<512> *pma_in1_ddr,
    ap_uint<512> *pma_out0_ddr,
    ap_uint<512> *pma_out1_ddr,
    hls::stream<pipe_type_96> &update_stream) {
#pragma HLS INTERFACE m_axi port = pma_in0_ddr offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = pma_in1_ddr offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = pma_out0_ddr offset = slave bundle = gmem2
#pragma HLS INTERFACE m_axi port = pma_out1_ddr offset = slave bundle = gmem3
#pragma HLS INTERFACE s_axilite port = pma_in0_ddr bundle = control
#pragma HLS INTERFACE s_axilite port = pma_in1_ddr bundle = control
#pragma HLS INTERFACE s_axilite port = pma_out0_ddr bundle = control
#pragma HLS INTERFACE s_axilite port = pma_out1_ddr bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS INTERFACE axis port=update_stream

    hls::stream<ap_uint<96> > update_stream_ddr_0[UPDATE_STREAM_DDR_NUM];
    hls::stream<ap_uint<96> > update_stream_ddr_1[UPDATE_STREAM_DDR_NUM];

#pragma HLS DATAFLOW
    dispatch_update(update_stream, update_stream_ddr_0, update_stream_ddr_1);
    process(pma_in0_ddr, pma_out0_ddr, update_stream_ddr_0);
    process(pma_in1_ddr, pma_out1_ddr, update_stream_ddr_1);
}
}
