#ifndef __KERNEL_CONFIG__
#define __KERNEL_CONFIG__

#define MAX_SEGMENT_SIZE 16
#define MAX_TREE_HEIGHT 32
#define DATA_CACHE_SIZE 128
#define ROW_OFFSET_CACHE_SIZE 128
#define DISPATCH_DATA_CACHE_SIZE 4096
#define GUARD (0xFFFFFFFFUL)
#define EMPTY (0x8000000000000000UL)
#define END_EDGE (0xFFFFFFFFFFFFFFFFUL)

#include <hls_stream.h>
#include <stdio.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>

typedef ap_axiu<64, 0, 0, 0> pipe_type;
typedef ap_axiu<96, 0, 0, 0> pipe_type_96;

#endif