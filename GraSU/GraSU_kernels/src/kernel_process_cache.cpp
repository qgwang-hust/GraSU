#include "kernel_config.h"

#define UPDATE_STREAM_CACHE_COUNT 16
#define UPDATE_STREAM_CACHE_COUNT_LOG 4

void dispatch_update(
    hls::stream<pipe_type_96> &update_stream,
    hls::stream<ap_uint<96> > update_stream_cache[UPDATE_STREAM_CACHE_COUNT]) {

    ap_uint<96> data;
    dispatch_loop:
    while (true) {
        #pragma HLS PIPELINE II=1
        data = update_stream.read().data;
        if (data == END_EDGE)
            break;
        update_stream_cache[data.range(4 + UPDATE_STREAM_CACHE_COUNT_LOG, 5)].write(data);
    }
    for (int i = 0; i < UPDATE_STREAM_CACHE_COUNT; i++) {
        #pragma HLS PIPELINE II=1
        update_stream_cache[i].write(END_EDGE);
    }
}

void process_1(
    ap_uint<512> *pma,
    hls::stream<ap_uint<96> > &update_stream) {

    ap_uint<512> data;
    ap_uint<512> res_data;
    ap_uint<96> info;
    ap_uint<32> local_edges[16];
    ap_uint<32> update_edge;
    ap_uint<32> index;
    ap_uint<10> segment_count;
    ap_uint<4> cmp_res[16];
    ap_uint<1> delete_op;

    process_loop:
    while (true) {
        info = update_stream.read();
        if (info == END_EDGE)
            break;
        index = info.range(31, 5 + UPDATE_STREAM_CACHE_COUNT_LOG);  // 每个segment包含16个值，舍去末4位，有两个process_cache的kernel，再舍去1位，本地有16条流水线并行，再舍去4位
        update_edge = info.range(63, 32);
        delete_op = info[95];
        segment_count = 0;

        data = pma[index];

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

        pma[index] = res_data;
    }
}

void process(
    ap_uint<512> *pma,
    hls::stream<ap_uint<96> > update_stream_cache[UPDATE_STREAM_CACHE_COUNT_LOG]) {

    ap_uint<512> pma_cache_0[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_1[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_2[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_3[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_4[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_5[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_6[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_7[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_8[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_9[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_A[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_B[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_C[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_D[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_E[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];
    ap_uint<512> pma_cache_F[MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG];

    #pragma HLS bind_storage variable=pma_cache_0 type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_1 type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_2 type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_3 type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_4 type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_5 type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_6 type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_7 type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_8 type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_9 type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_A type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_B type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_C type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_D type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_E type=RAM_1P impl=URAM
    #pragma HLS bind_storage variable=pma_cache_F type=RAM_1P impl=URAM

//	#pragma HLS RESOURCE variable=pma_cache_0 core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_1 core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_2 core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_3 core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_4 core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_5 core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_6 core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_7 core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_8 core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_9 core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_A core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_B core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_C core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_D core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_E core=XPM_MEMORY uram
//	#pragma HLS RESOURCE variable=pma_cache_F core=XPM_MEMORY uram

    load_cache:
    for (int i = 0; i < (MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG); i++) {
        #pragma HLS PIPELINE II=16
        pma_cache_0[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x0];
        pma_cache_1[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x1];
        pma_cache_2[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x2];
        pma_cache_3[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x3];
        pma_cache_4[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x4];
        pma_cache_5[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x5];
        pma_cache_6[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x6];
        pma_cache_7[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x7];
        pma_cache_8[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x8];
        pma_cache_9[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x9];
        pma_cache_A[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xA];
        pma_cache_B[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xB];
        pma_cache_C[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xC];
        pma_cache_D[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xD];
        pma_cache_E[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xE];
        pma_cache_F[i] = pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xF];
    }

    process_1(pma_cache_0, update_stream_cache[0x0]);
    process_1(pma_cache_1, update_stream_cache[0x1]);
    process_1(pma_cache_2, update_stream_cache[0x2]);
    process_1(pma_cache_3, update_stream_cache[0x3]);
    process_1(pma_cache_4, update_stream_cache[0x4]);
    process_1(pma_cache_5, update_stream_cache[0x5]);
    process_1(pma_cache_6, update_stream_cache[0x6]);
    process_1(pma_cache_7, update_stream_cache[0x7]);
    process_1(pma_cache_8, update_stream_cache[0x8]);
    process_1(pma_cache_9, update_stream_cache[0x9]);
    process_1(pma_cache_A, update_stream_cache[0xA]);
    process_1(pma_cache_B, update_stream_cache[0xB]);
    process_1(pma_cache_C, update_stream_cache[0xC]);
    process_1(pma_cache_D, update_stream_cache[0xD]);
    process_1(pma_cache_E, update_stream_cache[0xE]);
    process_1(pma_cache_F, update_stream_cache[0xF]);

    store_cache:
    for (int i = 0; i < (MAX_CACHE_SEGMENT >> UPDATE_STREAM_CACHE_COUNT_LOG); i++) {
        #pragma HLS PIPELINE II=16
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x0] = pma_cache_0[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x1] = pma_cache_1[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x2] = pma_cache_2[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x3] = pma_cache_3[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x4] = pma_cache_4[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x5] = pma_cache_5[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x6] = pma_cache_6[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x7] = pma_cache_7[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x8] = pma_cache_8[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0x9] = pma_cache_9[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xA] = pma_cache_A[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xB] = pma_cache_B[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xC] = pma_cache_C[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xD] = pma_cache_D[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xE] = pma_cache_E[i];
        pma[(i << UPDATE_STREAM_CACHE_COUNT_LOG) + 0xF] = pma_cache_F[i];
    }
}

extern "C" {
void process_cache(
    ap_uint<512> *pma_cache,
    hls::stream<pipe_type_96> &update_stream) {
#pragma HLS INTERFACE m_axi port = pma_cache offset = slave bundle = gmem0
#pragma HLS INTERFACE s_axilite port = pma_cache bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS INTERFACE axis port=update_stream

    hls::stream<ap_uint<96> > update_stream_cache[UPDATE_STREAM_CACHE_COUNT];

#pragma HLS DATAFLOW
    dispatch_update(update_stream, update_stream_cache);
    process(pma_cache, update_stream_cache);
}
}
