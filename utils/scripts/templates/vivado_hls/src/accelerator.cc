#include "../inc/<accelerator_name>.h"
#include "../inc/type.h"
#include "hls_stream.h"
#include "hls_math.h"
#include <cstring>

void <accelerator_name>_load(word _inbuff[CHUNK_SIZE], word *in1, unsigned chunk,
		      dma_info_t *store_ctrl, int base_index);
void <accelerator_name>_store(word _outbuff[CHUNK_SIZE], word *out, unsigned chunk,
		       dma_info_t *load_ctrl, int base_index);
void compute(word *_inbuff, word *_outbuff);

void <accelerator_name>_basic_dma32(word *out, word *in1, const unsigned conf_info_size,
		      dma_info_t *load_ctrl, dma_info_t *store_ctrl)
{
    word _inbuff[CHUNK_SIZE];
    word _outbuff[CHUNK_SIZE];

    // Right now conf_info_size must be a multiple of CHUNK_SIZE
    int n_chunks = conf_info_size / CHUNK_SIZE;

go:
    for (unsigned i = 0; i < n_chunks; i++)
    {
	<accelerator_name>_load(_inbuff, in1, i, load_ctrl, 0);
	compute(_inbuff, _outbuff);
	<accelerator_name>_store( _outbuff, out, i, store_ctrl, conf_info_size);
    }
}


void <accelerator_name>_load(word _inbuff[CHUNK_SIZE], word *in1, unsigned chunk,
		      dma_info_t *load_ctrl, int base_index)
{

load_data:

    unsigned base = CHUNK_SIZE * chunk;

    load_ctrl[chunk].index = base;
    load_ctrl[chunk].length = CHUNK_SIZE;
    load_ctrl[chunk].size = SIZE_WORD;

    for (unsigned i = 0; i < CHUNK_SIZE; i++)
	_inbuff[i] = in1[base + i];
}


void <accelerator_name>_store(word _outbuff[CHUNK_SIZE], word *out, unsigned chunk,
		       dma_info_t *store_ctrl, int base_index)
{

store_data:

    unsigned base = CHUNK_SIZE * chunk;

    store_ctrl[chunk].index = base + base_index;
    store_ctrl[chunk].length = CHUNK_SIZE;
    store_ctrl[chunk].size = SIZE_WORD;

    for (unsigned i = 0; i < CHUNK_SIZE; i++)
    	out[base + i] = _outbuff[i];
}


void compute(word *_inbuff, word *_outbuff)
{
    for (int i = 0; i < CHUNK_SIZE; i++)
    	_outbuff[i] = _inbuff[i];
}
