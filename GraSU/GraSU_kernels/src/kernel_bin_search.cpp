#include "kernel_config.h"
#include <ap_utils.h>

#define BIPA_PIPE_COUNT 16
#define BIPA_PIPE_COUNT_LOG 4

void read_edges(
    const unsigned long *edges,
    const unsigned int edge_size,
    hls::stream<ap_uint<64> > edge_stream[64]) {

    ap_uint<64> edge;
    read_edge_loop:
    for (int i = 0; i < edge_size; i++) {
        #pragma HLS PIPELINE II=1
        edge = edges[i];
        edge_stream[i & (64 - 1)].write(edge);
    }
    read_edge_fin_loop:
    for (int i = edge_size; i < edge_size + 64; i++) {
        #pragma HLS PIPELINE II=1
        edge_stream[i & (64 - 1)].write(END_EDGE);
    }
}

void binary_search(
    hls::stream<ap_uint<32> > &dram_binary_addr_fifo,
    hls::stream<ap_uint<64> > &dram_binary_data_fifo,
    hls::stream<ap_uint<32> > &dram_offset_addr_fifo,
    hls::stream<ap_uint<64> > &dram_offset_data_fifo,
    hls::stream<ap_uint<64> > &edge_stream,
    hls::stream<ap_uint<96> > &res_stream) {

    ap_uint<32> begin, end, mid;
    ap_uint<64> mid_data;
    ap_uint<64> edge;
    ap_uint<96> res;
    ap_uint<32> node_begin;
    ap_uint<1> finish = 0;

search_top_loop:
    while (finish == 0) {
        edge = edge_stream.read();
        if (edge == END_EDGE) {
            finish = 1;
        } else {
            res = edge;
            res <<= 32;
            edge &= (~EMPTY);
            node_begin = edge.range(63, 32);
            dram_offset_addr_fifo.write(node_begin);
            ap_wait();
            mid_data = dram_offset_data_fifo.read();
            begin = mid_data.range(63, 32);
            end = mid_data.range(31, 0);
            begin >>= 4;
            end >>= 4;
            mid = (begin + end) >> 1;
        binary_search_loop:
            while (begin != mid) {
                dram_binary_addr_fifo.write(mid);
                ap_wait();
                mid_data = dram_binary_data_fifo.read();
                if (mid_data <= edge) {
                    begin = mid;
                } else {
                    end = mid;
                }
                mid = (begin + end) >> 1;
            }
            res_stream.write(res + (begin << 4));
        }
    }
    dram_binary_addr_fifo.write(END_INDEX);
    dram_offset_addr_fifo.write(END_INDEX);
}

void bipa(
    const ap_uint<64> *dram_port,
    hls::stream<ap_uint<32> > dram_addr_fifo[BIPA_PIPE_COUNT],
    hls::stream<ap_uint<64> > dram_data_fifo[BIPA_PIPE_COUNT]) {

    // ap_uint<5> finish = 0;
    // ap_uint<1> finish[BIPA_PIPE_COUNT];
    // #pragma array_partition type=complete dim=1 variable=finish
    // #pragma bind_storage variable=finish type=RAM_1P impl=LUTRAM
    ap_uint<1> finish0 = 0;
    ap_uint<1> finish1 = 0;
    ap_uint<1> finish2 = 0;
    ap_uint<1> finish3 = 0;
    ap_uint<1> finish4 = 0;
    ap_uint<1> finish5 = 0;
    ap_uint<1> finish6 = 0;
    ap_uint<1> finish7 = 0;
    ap_uint<1> finish8 = 0;
    ap_uint<1> finish9 = 0;
    ap_uint<1> finishA = 0;
    ap_uint<1> finishB = 0;
    ap_uint<1> finishC = 0;
    ap_uint<1> finishD = 0;
    ap_uint<1> finishE = 0;
    ap_uint<1> finishF = 0;
    ap_uint<32> addr;
    ap_uint<64> data;
    ap_uint<4> cnt = 0;

    bipa_loop:
    while (finish0 == 0 || finish1 == 0 || finish2 == 0 || finish3 == 0 ||
           finish4 == 0 || finish5 == 0 || finish6 == 0 || finish7 == 0 ||
           finish8 == 0 || finish9 == 0 || finishA == 0 || finishB == 0 ||
           finishC == 0 || finishD == 0 || finishE == 0 || finishF == 0 || 0) {
        #pragma HLS PIPELINE II=1
        if (dram_addr_fifo[cnt].read_nb(addr)) {
            if (addr == END_INDEX) {
                switch (cnt) {
                case 0:
                    finish0 = 1;
                    break;
                case 1:
                    finish1 = 1;
                    break;
                case 2:
                    finish2 = 1;
                    break;
                case 3:
                    finish3 = 1;
                    break;
                case 4:
                    finish4 = 1;
                    break;
                case 5:
                    finish5 = 1;
                    break;
                case 6:
                    finish6 = 1;
                    break;
                case 7:
                    finish7 = 1;
                    break;
                case 8:
                    finish8 = 1;
                    break;
                case 9:
                    finish9 = 1;
                    break;
                case 10:
                    finishA = 1;
                    break;
                case 11:
                    finishB = 1;
                    break;
                case 12:
                    finishC = 1;
                    break;
                case 13:
                    finishD = 1;
                    break;
                case 14:
                    finishE = 1;
                    break;
                case 15:
                    finishF = 1;
                    break;
                }
            } else {
                data = dram_port[addr];
                dram_data_fifo[cnt].write(data);
            }
        }
        cnt++;
    }
}

void merge_updates(
    const unsigned int edge_size,
    hls::stream<ap_uint<96> > segment_head_stream[64],
    hls::stream<pipe_type_96> &segment_head_stream_out) {
    
    pipe_type_96 data;
    ap_uint<8> count = 0;
    ap_uint<7> cnt = 0;
merge_update_loop:
    for (int i = 0; i < edge_size; i++) {
        #pragma HLS PIPELINE II=1
        data.data = segment_head_stream[i & (64 - 1)].read();
        segment_head_stream_out.write(data);
    }
}

// 每个SLR上一个bin_search kernel，其中包含4条流水线
extern "C" {
void bin_search(
    const unsigned long *edges,     // 此SLR上需要处理的更新
    const ap_uint<64> *binary_0,
    const ap_uint<64> *binary_1,
    const ap_uint<64> *binary_2,
    const ap_uint<64> *binary_3,

    const ap_uint<64> *row_offset_0,
    const ap_uint<64> *row_offset_1,
    const ap_uint<64> *row_offset_2,
    const ap_uint<64> *row_offset_3,
    const unsigned int edge_size,   // 更新数量
    hls::stream<pipe_type_96> &segment_head_stream_out) {
#pragma HLS INTERFACE m_axi port = edges offset = slave bundle = gmem_edges
#pragma HLS INTERFACE s_axilite port = edges bundle = control
#pragma HLS INTERFACE s_axilite port = edge_size bundle = control

#pragma HLS INTERFACE m_axi port = binary_0 offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = binary_1 offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = binary_2 offset = slave bundle = gmem2
#pragma HLS INTERFACE m_axi port = binary_3 offset = slave bundle = gmem3

#pragma HLS INTERFACE m_axi port = row_offset_0 offset = slave bundle = gmem4
#pragma HLS INTERFACE m_axi port = row_offset_1 offset = slave bundle = gmem5
#pragma HLS INTERFACE m_axi port = row_offset_2 offset = slave bundle = gmem6
#pragma HLS INTERFACE m_axi port = row_offset_3 offset = slave bundle = gmem7

#pragma HLS INTERFACE s_axilite port = binary_0 bundle = control
#pragma HLS INTERFACE s_axilite port = binary_1 bundle = control
#pragma HLS INTERFACE s_axilite port = binary_2 bundle = control
#pragma HLS INTERFACE s_axilite port = binary_3 bundle = control

#pragma HLS INTERFACE s_axilite port = row_offset_0 bundle = control
#pragma HLS INTERFACE s_axilite port = row_offset_1 bundle = control
#pragma HLS INTERFACE s_axilite port = row_offset_2 bundle = control
#pragma HLS INTERFACE s_axilite port = row_offset_3 bundle = control

#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS INTERFACE axis port = segment_head_stream_out

    hls::stream<ap_uint<64>  > edge_stream[64];
    hls::stream<ap_uint<96>  > res_stream[64];

    hls::stream<ap_uint<32> > bipa_row_offset_dram_addr_fifo_0[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<32> > bipa_row_offset_dram_addr_fifo_1[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<32> > bipa_row_offset_dram_addr_fifo_2[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<32> > bipa_row_offset_dram_addr_fifo_3[BIPA_PIPE_COUNT];

    hls::stream<ap_uint<64> > bipa_row_offset_dram_data_fifo_0[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<64> > bipa_row_offset_dram_data_fifo_1[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<64> > bipa_row_offset_dram_data_fifo_2[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<64> > bipa_row_offset_dram_data_fifo_3[BIPA_PIPE_COUNT];

    hls::stream<ap_uint<32> > bipa_binary_dram_addr_fifo_0[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<32> > bipa_binary_dram_addr_fifo_1[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<32> > bipa_binary_dram_addr_fifo_2[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<32> > bipa_binary_dram_addr_fifo_3[BIPA_PIPE_COUNT];

    hls::stream<ap_uint<64> > bipa_binary_dram_data_fifo_0[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<64> > bipa_binary_dram_data_fifo_1[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<64> > bipa_binary_dram_data_fifo_2[BIPA_PIPE_COUNT];
    hls::stream<ap_uint<64> > bipa_binary_dram_data_fifo_3[BIPA_PIPE_COUNT];

#pragma HLS DATAFLOW
    read_edges(edges, edge_size, edge_stream);
    
    /* -------------------------------------------
     * 64个二分查找模块
     * ------------------------------------------- */
    binary_search(bipa_binary_dram_addr_fifo_0[0x0], bipa_binary_dram_data_fifo_0[0x0], bipa_row_offset_dram_addr_fifo_0[0x0], bipa_row_offset_dram_data_fifo_0[0x0], edge_stream[0x0], res_stream[0x0]);
    binary_search(bipa_binary_dram_addr_fifo_0[0x1], bipa_binary_dram_data_fifo_0[0x1], bipa_row_offset_dram_addr_fifo_0[0x1], bipa_row_offset_dram_data_fifo_0[0x1], edge_stream[0x1], res_stream[0x1]);
    binary_search(bipa_binary_dram_addr_fifo_0[0x2], bipa_binary_dram_data_fifo_0[0x2], bipa_row_offset_dram_addr_fifo_0[0x2], bipa_row_offset_dram_data_fifo_0[0x2], edge_stream[0x2], res_stream[0x2]);
    binary_search(bipa_binary_dram_addr_fifo_0[0x3], bipa_binary_dram_data_fifo_0[0x3], bipa_row_offset_dram_addr_fifo_0[0x3], bipa_row_offset_dram_data_fifo_0[0x3], edge_stream[0x3], res_stream[0x3]);
    binary_search(bipa_binary_dram_addr_fifo_0[0x4], bipa_binary_dram_data_fifo_0[0x4], bipa_row_offset_dram_addr_fifo_0[0x4], bipa_row_offset_dram_data_fifo_0[0x4], edge_stream[0x4], res_stream[0x4]);
    binary_search(bipa_binary_dram_addr_fifo_0[0x5], bipa_binary_dram_data_fifo_0[0x5], bipa_row_offset_dram_addr_fifo_0[0x5], bipa_row_offset_dram_data_fifo_0[0x5], edge_stream[0x5], res_stream[0x5]);
    binary_search(bipa_binary_dram_addr_fifo_0[0x6], bipa_binary_dram_data_fifo_0[0x6], bipa_row_offset_dram_addr_fifo_0[0x6], bipa_row_offset_dram_data_fifo_0[0x6], edge_stream[0x6], res_stream[0x6]);
    binary_search(bipa_binary_dram_addr_fifo_0[0x7], bipa_binary_dram_data_fifo_0[0x7], bipa_row_offset_dram_addr_fifo_0[0x7], bipa_row_offset_dram_data_fifo_0[0x7], edge_stream[0x7], res_stream[0x7]);
    binary_search(bipa_binary_dram_addr_fifo_0[0x8], bipa_binary_dram_data_fifo_0[0x8], bipa_row_offset_dram_addr_fifo_0[0x8], bipa_row_offset_dram_data_fifo_0[0x8], edge_stream[0x8], res_stream[0x8]);
    binary_search(bipa_binary_dram_addr_fifo_0[0x9], bipa_binary_dram_data_fifo_0[0x9], bipa_row_offset_dram_addr_fifo_0[0x9], bipa_row_offset_dram_data_fifo_0[0x9], edge_stream[0x9], res_stream[0x9]);
    binary_search(bipa_binary_dram_addr_fifo_0[0xA], bipa_binary_dram_data_fifo_0[0xA], bipa_row_offset_dram_addr_fifo_0[0xA], bipa_row_offset_dram_data_fifo_0[0xA], edge_stream[0xA], res_stream[0xA]);
    binary_search(bipa_binary_dram_addr_fifo_0[0xB], bipa_binary_dram_data_fifo_0[0xB], bipa_row_offset_dram_addr_fifo_0[0xB], bipa_row_offset_dram_data_fifo_0[0xB], edge_stream[0xB], res_stream[0xB]);
    binary_search(bipa_binary_dram_addr_fifo_0[0xC], bipa_binary_dram_data_fifo_0[0xC], bipa_row_offset_dram_addr_fifo_0[0xC], bipa_row_offset_dram_data_fifo_0[0xC], edge_stream[0xC], res_stream[0xC]);
    binary_search(bipa_binary_dram_addr_fifo_0[0xD], bipa_binary_dram_data_fifo_0[0xD], bipa_row_offset_dram_addr_fifo_0[0xD], bipa_row_offset_dram_data_fifo_0[0xD], edge_stream[0xD], res_stream[0xD]);
    binary_search(bipa_binary_dram_addr_fifo_0[0xE], bipa_binary_dram_data_fifo_0[0xE], bipa_row_offset_dram_addr_fifo_0[0xE], bipa_row_offset_dram_data_fifo_0[0xE], edge_stream[0xE], res_stream[0xE]);
    binary_search(bipa_binary_dram_addr_fifo_0[0xF], bipa_binary_dram_data_fifo_0[0xF], bipa_row_offset_dram_addr_fifo_0[0xF], bipa_row_offset_dram_data_fifo_0[0xF], edge_stream[0xF], res_stream[0xF]);

    binary_search(bipa_binary_dram_addr_fifo_1[0x0], bipa_binary_dram_data_fifo_1[0x0], bipa_row_offset_dram_addr_fifo_1[0x0], bipa_row_offset_dram_data_fifo_1[0x0], edge_stream[0x10], res_stream[0x10]);
    binary_search(bipa_binary_dram_addr_fifo_1[0x1], bipa_binary_dram_data_fifo_1[0x1], bipa_row_offset_dram_addr_fifo_1[0x1], bipa_row_offset_dram_data_fifo_1[0x1], edge_stream[0x11], res_stream[0x11]);
    binary_search(bipa_binary_dram_addr_fifo_1[0x2], bipa_binary_dram_data_fifo_1[0x2], bipa_row_offset_dram_addr_fifo_1[0x2], bipa_row_offset_dram_data_fifo_1[0x2], edge_stream[0x12], res_stream[0x12]);
    binary_search(bipa_binary_dram_addr_fifo_1[0x3], bipa_binary_dram_data_fifo_1[0x3], bipa_row_offset_dram_addr_fifo_1[0x3], bipa_row_offset_dram_data_fifo_1[0x3], edge_stream[0x13], res_stream[0x13]);
    binary_search(bipa_binary_dram_addr_fifo_1[0x4], bipa_binary_dram_data_fifo_1[0x4], bipa_row_offset_dram_addr_fifo_1[0x4], bipa_row_offset_dram_data_fifo_1[0x4], edge_stream[0x14], res_stream[0x14]);
    binary_search(bipa_binary_dram_addr_fifo_1[0x5], bipa_binary_dram_data_fifo_1[0x5], bipa_row_offset_dram_addr_fifo_1[0x5], bipa_row_offset_dram_data_fifo_1[0x5], edge_stream[0x15], res_stream[0x15]);
    binary_search(bipa_binary_dram_addr_fifo_1[0x6], bipa_binary_dram_data_fifo_1[0x6], bipa_row_offset_dram_addr_fifo_1[0x6], bipa_row_offset_dram_data_fifo_1[0x6], edge_stream[0x16], res_stream[0x16]);
    binary_search(bipa_binary_dram_addr_fifo_1[0x7], bipa_binary_dram_data_fifo_1[0x7], bipa_row_offset_dram_addr_fifo_1[0x7], bipa_row_offset_dram_data_fifo_1[0x7], edge_stream[0x17], res_stream[0x17]);
    binary_search(bipa_binary_dram_addr_fifo_1[0x8], bipa_binary_dram_data_fifo_1[0x8], bipa_row_offset_dram_addr_fifo_1[0x8], bipa_row_offset_dram_data_fifo_1[0x8], edge_stream[0x18], res_stream[0x18]);
    binary_search(bipa_binary_dram_addr_fifo_1[0x9], bipa_binary_dram_data_fifo_1[0x9], bipa_row_offset_dram_addr_fifo_1[0x9], bipa_row_offset_dram_data_fifo_1[0x9], edge_stream[0x19], res_stream[0x19]);
    binary_search(bipa_binary_dram_addr_fifo_1[0xA], bipa_binary_dram_data_fifo_1[0xA], bipa_row_offset_dram_addr_fifo_1[0xA], bipa_row_offset_dram_data_fifo_1[0xA], edge_stream[0x1A], res_stream[0x1A]);
    binary_search(bipa_binary_dram_addr_fifo_1[0xB], bipa_binary_dram_data_fifo_1[0xB], bipa_row_offset_dram_addr_fifo_1[0xB], bipa_row_offset_dram_data_fifo_1[0xB], edge_stream[0x1B], res_stream[0x1B]);
    binary_search(bipa_binary_dram_addr_fifo_1[0xC], bipa_binary_dram_data_fifo_1[0xC], bipa_row_offset_dram_addr_fifo_1[0xC], bipa_row_offset_dram_data_fifo_1[0xC], edge_stream[0x1C], res_stream[0x1C]);
    binary_search(bipa_binary_dram_addr_fifo_1[0xD], bipa_binary_dram_data_fifo_1[0xD], bipa_row_offset_dram_addr_fifo_1[0xD], bipa_row_offset_dram_data_fifo_1[0xD], edge_stream[0x1D], res_stream[0x1D]);
    binary_search(bipa_binary_dram_addr_fifo_1[0xE], bipa_binary_dram_data_fifo_1[0xE], bipa_row_offset_dram_addr_fifo_1[0xE], bipa_row_offset_dram_data_fifo_1[0xE], edge_stream[0x1E], res_stream[0x1E]);
    binary_search(bipa_binary_dram_addr_fifo_1[0xF], bipa_binary_dram_data_fifo_1[0xF], bipa_row_offset_dram_addr_fifo_1[0xF], bipa_row_offset_dram_data_fifo_1[0xF], edge_stream[0x1F], res_stream[0x1F]);

    binary_search(bipa_binary_dram_addr_fifo_2[0x0], bipa_binary_dram_data_fifo_2[0x0], bipa_row_offset_dram_addr_fifo_2[0x0], bipa_row_offset_dram_data_fifo_2[0x0], edge_stream[0x20], res_stream[0x20]);
    binary_search(bipa_binary_dram_addr_fifo_2[0x1], bipa_binary_dram_data_fifo_2[0x1], bipa_row_offset_dram_addr_fifo_2[0x1], bipa_row_offset_dram_data_fifo_2[0x1], edge_stream[0x21], res_stream[0x21]);
    binary_search(bipa_binary_dram_addr_fifo_2[0x2], bipa_binary_dram_data_fifo_2[0x2], bipa_row_offset_dram_addr_fifo_2[0x2], bipa_row_offset_dram_data_fifo_2[0x2], edge_stream[0x22], res_stream[0x22]);
    binary_search(bipa_binary_dram_addr_fifo_2[0x3], bipa_binary_dram_data_fifo_2[0x3], bipa_row_offset_dram_addr_fifo_2[0x3], bipa_row_offset_dram_data_fifo_2[0x3], edge_stream[0x23], res_stream[0x23]);
    binary_search(bipa_binary_dram_addr_fifo_2[0x4], bipa_binary_dram_data_fifo_2[0x4], bipa_row_offset_dram_addr_fifo_2[0x4], bipa_row_offset_dram_data_fifo_2[0x4], edge_stream[0x24], res_stream[0x24]);
    binary_search(bipa_binary_dram_addr_fifo_2[0x5], bipa_binary_dram_data_fifo_2[0x5], bipa_row_offset_dram_addr_fifo_2[0x5], bipa_row_offset_dram_data_fifo_2[0x5], edge_stream[0x25], res_stream[0x25]);
    binary_search(bipa_binary_dram_addr_fifo_2[0x6], bipa_binary_dram_data_fifo_2[0x6], bipa_row_offset_dram_addr_fifo_2[0x6], bipa_row_offset_dram_data_fifo_2[0x6], edge_stream[0x26], res_stream[0x26]);
    binary_search(bipa_binary_dram_addr_fifo_2[0x7], bipa_binary_dram_data_fifo_2[0x7], bipa_row_offset_dram_addr_fifo_2[0x7], bipa_row_offset_dram_data_fifo_2[0x7], edge_stream[0x27], res_stream[0x27]);
    binary_search(bipa_binary_dram_addr_fifo_2[0x8], bipa_binary_dram_data_fifo_2[0x8], bipa_row_offset_dram_addr_fifo_2[0x8], bipa_row_offset_dram_data_fifo_2[0x8], edge_stream[0x28], res_stream[0x28]);
    binary_search(bipa_binary_dram_addr_fifo_2[0x9], bipa_binary_dram_data_fifo_2[0x9], bipa_row_offset_dram_addr_fifo_2[0x9], bipa_row_offset_dram_data_fifo_2[0x9], edge_stream[0x29], res_stream[0x29]);
    binary_search(bipa_binary_dram_addr_fifo_2[0xA], bipa_binary_dram_data_fifo_2[0xA], bipa_row_offset_dram_addr_fifo_2[0xA], bipa_row_offset_dram_data_fifo_2[0xA], edge_stream[0x2A], res_stream[0x2A]);
    binary_search(bipa_binary_dram_addr_fifo_2[0xB], bipa_binary_dram_data_fifo_2[0xB], bipa_row_offset_dram_addr_fifo_2[0xB], bipa_row_offset_dram_data_fifo_2[0xB], edge_stream[0x2B], res_stream[0x2B]);
    binary_search(bipa_binary_dram_addr_fifo_2[0xC], bipa_binary_dram_data_fifo_2[0xC], bipa_row_offset_dram_addr_fifo_2[0xC], bipa_row_offset_dram_data_fifo_2[0xC], edge_stream[0x2C], res_stream[0x2C]);
    binary_search(bipa_binary_dram_addr_fifo_2[0xD], bipa_binary_dram_data_fifo_2[0xD], bipa_row_offset_dram_addr_fifo_2[0xD], bipa_row_offset_dram_data_fifo_2[0xD], edge_stream[0x2D], res_stream[0x2D]);
    binary_search(bipa_binary_dram_addr_fifo_2[0xE], bipa_binary_dram_data_fifo_2[0xE], bipa_row_offset_dram_addr_fifo_2[0xE], bipa_row_offset_dram_data_fifo_2[0xE], edge_stream[0x2E], res_stream[0x2E]);
    binary_search(bipa_binary_dram_addr_fifo_2[0xF], bipa_binary_dram_data_fifo_2[0xF], bipa_row_offset_dram_addr_fifo_2[0xF], bipa_row_offset_dram_data_fifo_2[0xF], edge_stream[0x2F], res_stream[0x2F]);

    binary_search(bipa_binary_dram_addr_fifo_3[0x0], bipa_binary_dram_data_fifo_3[0x0], bipa_row_offset_dram_addr_fifo_3[0x0], bipa_row_offset_dram_data_fifo_3[0x0], edge_stream[0x30], res_stream[0x30]);
    binary_search(bipa_binary_dram_addr_fifo_3[0x1], bipa_binary_dram_data_fifo_3[0x1], bipa_row_offset_dram_addr_fifo_3[0x1], bipa_row_offset_dram_data_fifo_3[0x1], edge_stream[0x31], res_stream[0x31]);
    binary_search(bipa_binary_dram_addr_fifo_3[0x2], bipa_binary_dram_data_fifo_3[0x2], bipa_row_offset_dram_addr_fifo_3[0x2], bipa_row_offset_dram_data_fifo_3[0x2], edge_stream[0x32], res_stream[0x32]);
    binary_search(bipa_binary_dram_addr_fifo_3[0x3], bipa_binary_dram_data_fifo_3[0x3], bipa_row_offset_dram_addr_fifo_3[0x3], bipa_row_offset_dram_data_fifo_3[0x3], edge_stream[0x33], res_stream[0x33]);
    binary_search(bipa_binary_dram_addr_fifo_3[0x4], bipa_binary_dram_data_fifo_3[0x4], bipa_row_offset_dram_addr_fifo_3[0x4], bipa_row_offset_dram_data_fifo_3[0x4], edge_stream[0x34], res_stream[0x34]);
    binary_search(bipa_binary_dram_addr_fifo_3[0x5], bipa_binary_dram_data_fifo_3[0x5], bipa_row_offset_dram_addr_fifo_3[0x5], bipa_row_offset_dram_data_fifo_3[0x5], edge_stream[0x35], res_stream[0x35]);
    binary_search(bipa_binary_dram_addr_fifo_3[0x6], bipa_binary_dram_data_fifo_3[0x6], bipa_row_offset_dram_addr_fifo_3[0x6], bipa_row_offset_dram_data_fifo_3[0x6], edge_stream[0x36], res_stream[0x36]);
    binary_search(bipa_binary_dram_addr_fifo_3[0x7], bipa_binary_dram_data_fifo_3[0x7], bipa_row_offset_dram_addr_fifo_3[0x7], bipa_row_offset_dram_data_fifo_3[0x7], edge_stream[0x37], res_stream[0x37]);
    binary_search(bipa_binary_dram_addr_fifo_3[0x8], bipa_binary_dram_data_fifo_3[0x8], bipa_row_offset_dram_addr_fifo_3[0x8], bipa_row_offset_dram_data_fifo_3[0x8], edge_stream[0x38], res_stream[0x38]);
    binary_search(bipa_binary_dram_addr_fifo_3[0x9], bipa_binary_dram_data_fifo_3[0x9], bipa_row_offset_dram_addr_fifo_3[0x9], bipa_row_offset_dram_data_fifo_3[0x9], edge_stream[0x39], res_stream[0x39]);
    binary_search(bipa_binary_dram_addr_fifo_3[0xA], bipa_binary_dram_data_fifo_3[0xA], bipa_row_offset_dram_addr_fifo_3[0xA], bipa_row_offset_dram_data_fifo_3[0xA], edge_stream[0x3A], res_stream[0x3A]);
    binary_search(bipa_binary_dram_addr_fifo_3[0xB], bipa_binary_dram_data_fifo_3[0xB], bipa_row_offset_dram_addr_fifo_3[0xB], bipa_row_offset_dram_data_fifo_3[0xB], edge_stream[0x3B], res_stream[0x3B]);
    binary_search(bipa_binary_dram_addr_fifo_3[0xC], bipa_binary_dram_data_fifo_3[0xC], bipa_row_offset_dram_addr_fifo_3[0xC], bipa_row_offset_dram_data_fifo_3[0xC], edge_stream[0x3C], res_stream[0x3C]);
    binary_search(bipa_binary_dram_addr_fifo_3[0xD], bipa_binary_dram_data_fifo_3[0xD], bipa_row_offset_dram_addr_fifo_3[0xD], bipa_row_offset_dram_data_fifo_3[0xD], edge_stream[0x3D], res_stream[0x3D]);
    binary_search(bipa_binary_dram_addr_fifo_3[0xE], bipa_binary_dram_data_fifo_3[0xE], bipa_row_offset_dram_addr_fifo_3[0xE], bipa_row_offset_dram_data_fifo_3[0xE], edge_stream[0x3E], res_stream[0x3E]);
    binary_search(bipa_binary_dram_addr_fifo_3[0xF], bipa_binary_dram_data_fifo_3[0xF], bipa_row_offset_dram_addr_fifo_3[0xF], bipa_row_offset_dram_data_fifo_3[0xF], edge_stream[0x3F], res_stream[0x3F]);

    bipa(row_offset_0, bipa_row_offset_dram_addr_fifo_0, bipa_row_offset_dram_data_fifo_0);
    bipa(row_offset_1, bipa_row_offset_dram_addr_fifo_1, bipa_row_offset_dram_data_fifo_1);
    bipa(row_offset_2, bipa_row_offset_dram_addr_fifo_2, bipa_row_offset_dram_data_fifo_2);
    bipa(row_offset_3, bipa_row_offset_dram_addr_fifo_3, bipa_row_offset_dram_data_fifo_3);

    bipa(binary_0, bipa_binary_dram_addr_fifo_0, bipa_binary_dram_data_fifo_0);
    bipa(binary_1, bipa_binary_dram_addr_fifo_1, bipa_binary_dram_data_fifo_1);
    bipa(binary_2, bipa_binary_dram_addr_fifo_2, bipa_binary_dram_data_fifo_2);
    bipa(binary_3, bipa_binary_dram_addr_fifo_3, bipa_binary_dram_data_fifo_3);

    merge_updates(edge_size, res_stream, segment_head_stream_out);
}
}
