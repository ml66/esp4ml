/**
 * Baremetal device driver for ADDER
 *
 * Select Scatter-Gather in ESP configuration
 */

#include <stdio.h>
#include <stdlib.h>
#include <esp_accelerator.h>
#include <esp_probe.h>

#define SLD_ADDER   0x14
#define DEV_NAME "sld,adder"

#define SIZE 256

#define ADDER_BUF_SIZE (SIZE * 4 * sizeof(int))
#define ADDER_OUT_SIZE (SIZE * sizeof(int))

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 9
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK ((ADDER_BUF_SIZE % CHUNK_SIZE == 0) ?		\
		(ADDER_BUF_SIZE / CHUNK_SIZE) :			\
		(ADDER_BUF_SIZE / CHUNK_SIZE) + 1)

// User defined registers
#define ADDER_SIZE_REG   0x40

// TODO store data in memory as fixed point

#define N_INVOKE 2 

int main(int argc, char * argv[])
{
    int n, k;
    int ndev;
    struct esp_device *espdevs = NULL;
    unsigned coherence;

    printf("STARTING!\n");

    ndev = probe(&espdevs, SLD_ADDER, DEV_NAME);
    if (!ndev) {
	printf("Error: device not found!");
	printf(DEV_NAME);
	exit(EXIT_FAILURE);
    }

    for (n = 0; n < ndev; n++) {

	for (k = 0; k < N_INVOKE; ++k) {

	    coherence = ACC_COH_NONE;

	    struct esp_device *dev = &espdevs[n];
	    int done;
	    int i;
	    unsigned **ptable;
	    unsigned int *mem;
	    unsigned errors = 0;
	    int scatter_gather = 1;

	    printf("******************** %s.%d ********************\n");

	    // Check access ok (TODO)

	    // Check if scatter-gather DMA is disabled
	    if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
		printf("  -> scatter-gather DMA is disabled; revert to contiguous buffer.\n");
		scatter_gather = 0;
	    } else {
		printf("  -> scatter-gather DMA is enabled.\n");
	    }

	    if (scatter_gather) {
		if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK) {
		    printf("  Trying to allocate # chunks on # TLB available entries: \n");
		    print_uart_int((uint32_t) NCHUNK);
		    printf("\t");
		    print_uart_int((uint32_t) ioread32(dev, PT_NCHUNK_MAX_REG));
		    printf("\n");
		    break;
		}
	    }

	    // Allocate memory (will be contiguos anyway in baremetal)
	    mem = aligned_malloc(ADDER_BUF_SIZE);
	    printf("  memory buffer base-address = ");
	    print_uart_addr((uint64_t) mem);

	    if (scatter_gather) {
		// Allocate and populate page table
		ptable = aligned_malloc(NCHUNK * sizeof(unsigned *));
		for (i = 0; i < NCHUNK; i++)
		    ptable[i] = (unsigned *)
			&mem[i * (CHUNK_SIZE / sizeof(unsigned))];
		printf("  ptable = ");
		print_uart_addr((uint64_t) ptable);
		printf("\n  nchunk = ");
		print_uart_int((uint32_t) NCHUNK);
		printf("\n");
	    }

	    // Initialize input
	    for (i = 0; i < SIZE * 2; ++i)
		mem[i] = i + k * SIZE * 2;

	    // Initialize output
	    for (; i < SIZE * 3; ++i)
		mem[i] = 63;

	    // Configure device
	    iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	    iowrite32(dev, COHERENCE_REG, coherence);

	    if (scatter_gather) {
		iowrite32(dev, PT_ADDRESS_REG, (unsigned) ptable);
		iowrite32(dev, PT_NCHUNK_REG, NCHUNK);
		iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
		iowrite32(dev, SRC_OFFSET_REG, 0);
		iowrite32(dev, DST_OFFSET_REG, 0);
	    } else {
		iowrite32(dev, SRC_OFFSET_REG, (unsigned) mem);
		iowrite32(dev, DST_OFFSET_REG, (unsigned) mem);
	    }

	    // Accelerator-specific registers
	    iowrite32(dev, ADDER_SIZE_REG, SIZE/2);

	    // Flush for non-coherent DMA
	    esp_flush(coherence);

	    // Start accelerator
	    printf("  Start..\n");
	    iowrite32(dev, CMD_REG, CMD_MASK_START);

	    done = 0;

	    while (!done) {
		done = ioread32(dev, STATUS_REG);
		done &= STATUS_MASK_DONE;
	    } 

	    iowrite32(dev, CMD_REG, 0x0);
	    printf("  Done\n");

	    /* Validation */
	    printf("  validating...\n");
			
	    // Allocate memory for gold output
	    int *mem_gold;
	    mem_gold = aligned_malloc(ADDER_OUT_SIZE);
	    printf("  memory buffer base-address = ");
	    print_uart_addr((uint64_t) mem_gold);
	    printf("\n");

	    // Populate memory for gold output
	    int val = 2;
	    i = 0;
	    while (i < SIZE) {
		mem_gold[i++] = val;
		val += 2;
		mem_gold[i++] = val;
		val += 6;
	    }

	    // printing the first few outputs
	    /* for (i = 0; i < SIZE * 4; i++) { */
	    for (i = SIZE * 2; i < SIZE * 2 + 64; i++) {
		printf("i mem mem_gold\n");
		print_uart_int((uint32_t) i);
		printf("\n");
		print_uart_int((uint32_t) mem[i]);
		printf("\n");
		print_uart_int((uint32_t) mem_gold[i - SIZE*2]);
		printf("\n");
	    }
	}
    }

    return 0;
}
