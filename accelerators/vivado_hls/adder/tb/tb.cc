#include "../inc/adder.h"
#include "../inc/type.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define nchunk 1

#define nSAMPLE (nchunk*CHUNK)

int main(int argc, char **argv) {

    printf("****start*****\n");

    fx_pt *data1f=(fx_pt*) malloc(nSAMPLE * 2 * sizeof(fx_pt));
    fx_pt *out1f= (fx_pt*) malloc(nSAMPLE * sizeof(fx_pt));

    if (out1f == NULL || data1f == NULL)
    {
    	printf("null operator...FAIL");
    	exit(1);
    }

    dma_info_t *load = (dma_info_t*) malloc(nSAMPLE / CHUNK * sizeof(dma_info_t));
    dma_info_t *store = (dma_info_t*) malloc(nSAMPLE / CHUNK * sizeof(dma_info_t));

    for(unsigned i = 0; i < nSAMPLE * 2; i++)
    	data1f[i] = (fx_pt) 2.2;

    for(unsigned i = 0; i < nSAMPLE; i++)
    	out1f[i] = (fx_pt) 3.3;

    //call the TOP function
    adder_basic_dma32(out1f, data1f, nSAMPLE, load, store);

    // display output
    for(unsigned i = 0; i < nSAMPLE; i++)
    	printf("out : %d : %f\n", i, out1f[i].to_float());

    free(data1f);
    free(out1f);
    free(load);
    free(store);

    return 0;
}
