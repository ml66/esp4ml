#include <iostream>
#include "espacc.h"
#include "type.h"
#include "parameters.h"
#include "myproject.h"
#include "myproject_block_layers.h"
#include "hls_stream.h"
#include "hls_math.h"
#include <cstring>

void LOAD (in_data_word _inbuff[SIZE_IN_CHUNK_DATA], dma_word_t *in1, unsigned chunk,
	  dma_info_t *load_ctrl, int base_index)
{
load_data:

    unsigned base = SIZE_IN_CHUNK * chunk;

    load_ctrl[chunk].index = base;
    load_ctrl[chunk].length = SIZE_IN_CHUNK;
    load_ctrl[chunk].size = SIZE_BYTE;

    for (unsigned i = 0; i < SIZE_IN_CHUNK; i++) {
    	load_label0:for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
	    _inbuff[i * VALUES_PER_WORD + j] = in1[base + i].word[j];;
    	}
    }
}

void STORE (out_data_word _outbuff[SIZE_OUT_CHUNK_CEIL], dma_word_t *out, unsigned chunk,
	    dma_info_t *store_ctrl, int base_index)
{
store_data:

    unsigned base = SIZE_OUT_CHUNK * chunk;

    store_ctrl[chunk].index = base + base_index;
    store_ctrl[chunk].length = SIZE_OUT_CHUNK;
    store_ctrl[chunk].size = SIZE_BYTE;

    for (unsigned i = 0; i < SIZE_OUT_CHUNK_CEIL; i++) {
	store_label1:for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
	    out[base + i].word[j] = _outbuff[i * VALUES_PER_WORD + j];
	}
    }
}

void COMPUTE (in_data_word *_inbuff, out_data_word *_outbuff)
{
    // computation
    unsigned short size_in1, size_out1;
#if (BLOCK == 1)
    block_layer1(_inbuff, _outbuff);
#elif (BLOCK == 2)
    block_layer2(_inbuff, _outbuff);
#elif (BLOCK == 3)
    block_layer3(_inbuff, _outbuff);
#elif (BLOCK == 4)
    block_layer4(_inbuff, _outbuff);
#elif (BLOCK == 5)
    block_layer5(_inbuff, _outbuff);
#endif    
}

void TOP (dma_word_t *out, dma_word_t *in1, const unsigned conf_info_ninputs,
	  dma_info_t *load_ctrl, dma_info_t *store_ctrl)

{
go:
    for (unsigned i = 0; i < conf_info_ninputs; i++)
    {
	int size_in = conf_info_ninputs * SIZE_IN_CHUNK;
	in_data_word _inbuff[SIZE_IN_CHUNK_DATA];
	out_data_word _outbuff[SIZE_OUT_CHUNK_CEIL];

	LOAD(_inbuff, in1, i, load_ctrl, 0);
	COMPUTE(_inbuff, _outbuff);
	STORE( _outbuff, out, i, store_ctrl, size_in);
    }
}
