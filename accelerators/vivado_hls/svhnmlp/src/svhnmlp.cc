#include "svhnmlp.h"
#include "type.h"
#include "parameters.h"
#include "myproject.h"
#include "hls_stream.h"
#include "hls_math.h"
#include <cstring>


void LOAD (in_data_word _inbuff[SIZE_IN_CHUNK_DATA], word *in1, unsigned chunk,
	  dma_info_t *load_ctrl, int base_index)
{

    printf("Load data\n");

load_data:

    unsigned base = SIZE_IN_CHUNK * chunk;

    load_ctrl[chunk].index = base;
    load_ctrl[chunk].length = SIZE_IN_CHUNK;
    load_ctrl[chunk].size = SIZE_WORD;

    unsigned k = 0;
    for (unsigned i = 0; i < SIZE_IN_CHUNK; i++) {
	for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
	    _inbuff[k++] = (in_data_word)
		in1[base + i].range(DATA_BITWIDTH * (j+1) - 1, DATA_BITWIDTH * j);
	}
    }
}

void STORE (out_data_word _outbuff[SIZE_OUT_CHUNK_DATA], word *out, unsigned chunk,
	    dma_info_t *store_ctrl, int base_index)
{

    printf("Store data\n");

store_data:

    unsigned base = SIZE_OUT_CHUNK * chunk;

    store_ctrl[chunk].index = base + base_index;
    store_ctrl[chunk].length = SIZE_OUT_CHUNK;
    store_ctrl[chunk].size = SIZE_WORD;

    unsigned i = 0, k = 0;
    for (; i < SIZE_OUT_CHUNK; i++) {
	word out_int = 0; 	
	for(unsigned j = 0; j < VALUES_PER_WORD; j++) {
	    if (k < SIZE_IN_CHUNK_DATA) {
		out_int.range(DATA_BITWIDTH * (j+1) - 1, DATA_BITWIDTH * j) =
		    (bus_data_word) _outbuff[k++];
	    }
	}
	out[base + i] = out_int;
    }
}

void COMPUTE (in_data_word *_inbuff, out_data_word *_outbuff)
{
    printf("Compute\n");

    // computation
    unsigned short size_in1, size_out1;
    myproject(_inbuff, _outbuff, size_in1, size_out1);
}

void TOP (word *out, word *in1, const unsigned n_chunks,
	  dma_info_t *load_ctrl, dma_info_t *store_ctrl)

{
    in_data_word _inbuff[SIZE_IN_CHUNK_DATA];
    out_data_word _outbuff[SIZE_OUT_CHUNK_DATA];

    printf("Main loop starting...\n");

go:
    for (unsigned i = 0; i < n_chunks; i++)
    {
	LOAD(_inbuff, in1, i, load_ctrl, 0);
	COMPUTE(_inbuff, _outbuff);
	STORE( _outbuff, out, i, store_ctrl, SIZE_IN);
    }
}
