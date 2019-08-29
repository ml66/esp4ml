#include "../inc/adder.h"
#include "../inc/type.h"
#include "hls_stream.h"
#include "hls_math.h"
#include <cstring>

void adder_load(fx_pt _inbuff[CHUNK * 2], fx_pt *in1, unsigned chunk,
	  dma_info_t *store_ctrl, int base_index);
void adder_store(fx_pt _outbuff[CHUNK], fx_pt *out, unsigned chunk,
	   dma_info_t *load_ctrl, int base_index);
void compute(fx_pt *_inbuff, fx_pt *_outbuff);

void adder_basic_dma32(fx_pt *out, fx_pt *in1, const unsigned conf_info_size,
		dma_info_t *load_ctrl, dma_info_t *store_ctrl)
{
    fx_pt _inbuff[CHUNK * 2];
    fx_pt _outbuff[CHUNK];

    // TO DO: support conf_info_size not multiple of CHUNK
    int n_chunks = conf_info_size / CHUNK;

go:
    for (unsigned i = 0; i < n_chunks; i++)
    {
	adder_load(_inbuff, in1, i, load_ctrl, 0);
	compute(_inbuff, _outbuff);
	adder_store( _outbuff, out, i, store_ctrl, conf_info_size * 2);
    }
}


void adder_load(fx_pt _inbuff[CHUNK * 2], fx_pt *in1, unsigned chunk,
	  dma_info_t *load_ctrl, int base_index){

load_data:

    unsigned chunk_size = CHUNK * 2;
    unsigned base = chunk_size * chunk;

    load_ctrl[chunk].index = base;
    load_ctrl[chunk].length = chunk_size;
    load_ctrl[chunk].size = SIZE_WORD;

    for (unsigned i = 0; i < CHUNK * 2; i++)
	_inbuff[i] = in1[base + i];
}


void adder_store(fx_pt _outbuff[CHUNK], fx_pt *out, unsigned chunk,
	   dma_info_t *store_ctrl, int base_index) {

store_data:

    unsigned base = CHUNK * chunk;

    store_ctrl[chunk].index = base + base_index;
    store_ctrl[chunk].length = CHUNK;
    store_ctrl[chunk].size = SIZE_WORD;

    for (unsigned i = 0; i < CHUNK; i++)
    	out[base + i] = _outbuff[i];
}


void compute(fx_pt *_inbuff, fx_pt *_outbuff){

    for (int i = 0; i < CHUNK; i++)
    	_outbuff[i] = _inbuff[i*2] + _inbuff[i*2+1];

}
