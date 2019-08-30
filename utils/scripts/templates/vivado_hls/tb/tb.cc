#include "../inc/<accelerator_name>.h"
#include "../inc/type.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define NCHUNK 1
#define SIZE (NCHUNK * CHUNK_SIZE)

int main(int argc, char **argv) {

    printf("\n**** Start ****\n");

    word *in  = (word*) malloc(SIZE * sizeof(word));
    word *out = (word*) malloc(SIZE * sizeof(word));
    dma_info_t *load  = (dma_info_t*) malloc(NCHUNK * sizeof(dma_info_t));
    dma_info_t *store = (dma_info_t*) malloc(NCHUNK * sizeof(dma_info_t));

    if (in == NULL || out == NULL || load == NULL || store == NULL)
    {
    	printf("null operator...FAIL");
    	exit(1);
    }

    // Prepare input data
    for(unsigned i = 0; i < SIZE; i++)
    	in[i] = (word) 1.1;

    // Call the TOP function
#ifdef DMA32
    <accelerator_name>_dma32(out, in, SIZE, load, store);
#else
    <accelerator_name>_dma64(out, in, SIZE, load, store);
#endif

    // Display output
    for(unsigned i = 0; i < SIZE; i++)
    	printf("out : %d : %f\n", i, out[i].to_float());

    free(in);
    free(out);
    free(load);
    free(store);

    return 0;
}
