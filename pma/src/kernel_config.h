#ifndef __KERNEL_CONFIG__
#define __KERNEL_CONFIG__

#define EMPTY (0x8000000000000000UL)
#define END_EDGE (0xFFFFFFFFFFFFFFFFUL)

#define END_INDEX 0xFFFFFFFFU
#define READ_MASK 0x80000000U

#define MAX_CACHE_SEGMENT 131072
// #define MAX_CACHE_SEGMENT 40960

// #define MAX_CACHE_SEGMENT 128

#define SEGMENT_SIZE 16

#define OUTPUT_RESULT 0


#include <hls_stream.h>
#include <stdio.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>

typedef ap_axiu<96, 0, 0, 0> pipe_type_96;

#endif
