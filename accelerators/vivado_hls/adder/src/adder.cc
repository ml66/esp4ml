#include "../inc/adder.h"
#include "../inc/type.h"
#include "hls_stream.h"
#include "hls_math.h"
#include <cstring>

void LOAD(
        word _inbuff[CHUNK_SIZE * 2], word *in1, unsigned chunk,
        dma_info_t *load_ctrl, int base_index)
{

load_data:

    unsigned chunk_size = CHUNK_SIZE * 2;
    unsigned base = chunk_size * chunk;

    load_ctrl[chunk].index = base;
    load_ctrl[chunk].length = chunk_size;
    load_ctrl[chunk].size = SIZE_WORD;

    for (unsigned i = 0; i < CHUNK_SIZE * 2; i++)
        _inbuff[i] = in1[base + i];
}

void STORE(
        word _outbuff[CHUNK_SIZE], word *out, unsigned chunk,
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

void COMPUTE(
        word *_inbuff, word *_outbuff)
{

    for (int i = 0; i < CHUNK_SIZE; i++)
        _outbuff[i] = _inbuff[i*2] + _inbuff[i*2+1];

}

void TOP(
        word *out, word *in1, const unsigned conf_info_size,
        dma_info_t *load_ctrl, dma_info_t *store_ctrl)
{
    word _inbuff[CHUNK_SIZE * 2];
    word _outbuff[CHUNK_SIZE];

    // TO DO: support conf_info_size not multiple of CHUNK_SIZE
    int n_chunks = conf_info_size / CHUNK_SIZE;

go:
    for (unsigned i = 0; i < n_chunks; i++)
    {
        LOAD(_inbuff, in1, i, load_ctrl, 0);
        COMPUTE(_inbuff, _outbuff);
        STORE( _outbuff, out, i, store_ctrl, conf_info_size * 2);
    }
}
