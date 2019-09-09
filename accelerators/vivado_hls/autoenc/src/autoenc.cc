#include "autoenc.h"
#include "type.h"
#include "parameters.h"
#include "myproject.h"
#include "hls_stream.h"
#include "hls_math.h"
#include <cstring>


void LOAD (in_data_word _inbuff[SIZE_IN_CHUNK_DATA], dma_word_t *in1, unsigned base,
	  dma_info_t *load_ctrl, int base_index)
{

    printf("Load data\n");

load_data:

    load_ctrl[chunk].index = base;
    load_ctrl[chunk].length = SIZE_IN_CHUNK;
    load_ctrl[chunk].size = SIZE_BYTE;

    for (unsigned i = 0; i < SIZE_IN_CHUNK; i++) {
    	load_label0:for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
	    _inbuff[i * VALUES_PER_WORD + j] = in1[base + i].word[j];
    	}
    }
    std::cout << std::endl;
}

void STORE (out_data_word _outbuff[SIZE_OUT_CHUNK_DATA + 8], dma_word_t *out, unsigned base,
	    dma_info_t *store_ctrl, int base_index)
{

    printf("Store data\n");

store_data:

    store_ctrl[chunk].index = base + base_index;
    store_ctrl[chunk].length = SIZE_OUT_CHUNK;
    store_ctrl[chunk].size = SIZE_BYTE;

    for (unsigned i = 0; i < SIZE_OUT_CHUNK; i++) {
	store_label1:for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
	    out[base + i].word[j] = _outbuff[i * VALUES_PER_WORD + j];
	}
    }
}

void COMPUTE (in_data_word _inbuff[SIZE_IN_CHUNK_DATA],
	      out_data_word _outbuff[SIZE_OUT_CHUNK_DATA])
{
    printf("Compute\n");

    // computation
    unsigned short size_in1, size_out1;
    myproject(_inbuff, _outbuff, size_in1, size_out1);
}

void TOP (dma_word_t *out, dma_word_t *in1, const unsigned conf_info_ninputs,
	  dma_info_t *load_ctrl, dma_info_t *store_ctrl)

{
    printf("Main loop starting...\n");

go:
    for (unsigned i = 0; i < conf_info_ninputs; i++)
    {
	int size_in = conf_info_ninputs * SIZE_IN_CHUNK;
	in_data_word _inbuff[SIZE_IN_CHUNK_DATA];
	out_data_word _outbuff[SIZE_OUT_CHUNK_DATA];

	unsigned in_base = SIZE_IN_CHUNK * i;
	unsigned out_base = SIZE_OUT_CHUNK * i;

	LOAD(_inbuff, in1, in_base, load_ctrl, 0);
	COMPUTE(_inbuff, _outbuff);
	STORE( _outbuff, out, out_base, store_ctrl, size_in);
    }
}
