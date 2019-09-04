#ifndef INC_TYPE_H
#define INC_TYPE_H

#include <ap_fixed.h>
#include <ap_int.h>
#include "parameters.h"

// Data types and constants
#define DATA_BITWIDTH 16
#define VALUES_PER_WORD (DMA_SIZE / DATA_BITWIDTH)
#define NINPUTS 3
#define NCHUNK NINPUTS
#define SIZE_IN_CHUNK_DATA  N_INPUT_1_1
#define SIZE_OUT_CHUNK_DATA N_LAYER_10
#define SIZE_IN_CHUNK  (N_INPUT_1_1 / VALUES_PER_WORD)
#define SIZE_OUT_CHUNK (N_LAYER_10  / VALUES_PER_WORD)
#define SIZE_IN (NINPUTS  * SIZE_IN_CHUNK)
#define SIZE_OUT (NINPUTS * SIZE_OUT_CHUNK)

typedef ap_int<DMA_SIZE> word;
typedef ap_int<DATA_BITWIDTH> bus_data_word;
typedef input_t in_data_word;
typedef result_t out_data_word;

#define TOP top
#define LOAD load
#define STORE store
#define COMPUTE compute

// Constants

// Ctrl
typedef struct dma_info {
    ap_uint<32> index;
    ap_uint<32> length;
    ap_uint<32> size;
} dma_info_t;

// The 'size' variable of 'dma_info' indicates the bit-width of the words
// processed by the accelerator. Here are the encodings:
#define SIZE_BYTE   0
#define SIZE_HWORD  1
#define SIZE_WORD   2
#define SIZE_DWORD  3
#define SIZE_4WORD  4
#define SIZE_8WORD  5
#define SIZE_16WORD 6
#define SIZE_32WORD 7

#endif
