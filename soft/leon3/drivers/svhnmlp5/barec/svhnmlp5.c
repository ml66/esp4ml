/**
 * Baremetal device driver for SVHNMLP5
 *
 * (point-to-point communication test)
 */

#ifndef __riscv
#include <stdio.h>
#include <stdlib.h>
#endif

#include <esp_accelerator.h>
#include <esp_probe.h>

typedef long long unsigned u64;
typedef unsigned u32;

#define mask 0xFEED0BAC00000000L

#define SLD_SVHNMLP5   0x45
#define DEV_NAME "sld,svhnmlp5"

// Data types and constants
#define NINPUTS 3
#define SIZE_IN_CHUNK_DATA  1024
#define SIZE_OUT_CHUNK_DATA 10
#define SIZE_IN (NINPUTS * SIZE_IN_CHUNK_DATA)
#define SIZE_OUT (NINPUTS * (SIZE_OUT_CHUNK_DATA + 6))  
#define SIZE (SIZE_IN + SIZE_OUT)

typedef char token_t;

#define SVHNMLP5_INBUF_SIZE (SIZE_IN * sizeof(token_t))
#define SVHNMLP5_OUTBUF_SIZE (SIZE_OUT * sizeof(token_t))
#define SVHNMLP5_BUF_SIZE (SVHNMLP5_INBUF_SIZE + SVHNMLP5_OUTBUF_SIZE)

/* Size of the contiguous chunks for scatter/gather */
#define CHUNK_SHIFT 13
#define CHUNK_SIZE BIT(CHUNK_SHIFT)
#define NCHUNK ((SVHNMLP5_BUF_SIZE % CHUNK_SIZE == 0) ?			\
			(SVHNMLP5_BUF_SIZE / CHUNK_SIZE) :			\
			(SVHNMLP5_BUF_SIZE / CHUNK_SIZE) + 1)

// User defined registers
#define SVHNMLP5_NINPUTS_REG 0x40

static int validate_svhnmlp5(token_t *mem)
{
    int i, j;
    int rtn = 0;
    int base;
    token_t gold[SIZE_OUT];

    print_uart("include data gold\n");

#include "data/gold.c"

    print_uart("after include data gold\n");

    for (i = 0; i < NINPUTS; i++) {
	base = i * 16;
	for (j = 0; j < SIZE_OUT_CHUNK_DATA; j++) {
            print_uart_int(mem[base + j + SIZE_IN]);
	    print_uart("\n");
	    if (mem[base + j + SIZE_IN] != gold[base + j]) {
	    	print_uart("error\n");
	    	rtn++;
	    }
	}
    }

    print_uart("return\n");

    return rtn;
}

static void init_buf (token_t *mem, int tokens)
{
#include "data/data.c"
}


int main(int argc, char * argv[])
{
	int i;
	int n;
	int ndev;
	struct esp_device *espdevs;
	struct esp_device *dev;
	struct esp_device *srcs[4];
	unsigned done;
	unsigned **ptable;
	token_t *mem;
	unsigned errors = 0;

	print_uart("\nsize in: ");
	print_uart_int(SIZE_IN);
	print_uart("\n");


	printf("Scanning device tree... \n");
	ndev = probe(&espdevs, SLD_SVHNMLP5, DEV_NAME);

	// Check DMA capabilities
	dev = &espdevs[0];

	if (ioread32(dev, PT_NCHUNK_MAX_REG) == 0) {
	    printf("  -> scatter-gather DMA is disabled. Abort.\n");
	    return 0;
	}

	if (ioread32(dev, PT_NCHUNK_MAX_REG) < NCHUNK) {
	    printf("  -> Not enough TLB entries available. Abort.\n");
	    return 0;
	}

	// Allocate memory
	mem = aligned_malloc(SVHNMLP5_BUF_SIZE);
#ifndef __riscv
	printf("  memory buffer base-address = %p\n", mem);
#else
	print_uart("  memory buffer base-address = "); print_uart_addr((uintptr_t) mem); print_uart("\n");
#endif
	// Allocate and populate page table
	ptable = aligned_malloc(NCHUNK * sizeof(unsigned *));
	for (i = 0; i < NCHUNK; i++)
		ptable[i] = (unsigned *) &mem[i * (CHUNK_SIZE / sizeof(token_t))];
#ifndef __riscv
	printf("  ptable = %p\n", ptable);
	printf("  nchunk = %lu\n", NCHUNK);
#else
	print_uart("  ptable = "); print_uart_addr((uintptr_t) ptable); print_uart("\n");
	print_uart("  nchunk = "); print_uart_int(NCHUNK); print_uart("\n");
#endif

	printf("  Generate random input...\n");
	init_buf(mem, SIZE);

	// Pass common configuration parameters to accelerator
	dev = &espdevs[0];

	iowrite32(dev, SELECT_REG, ioread32(dev, DEVID_REG));
	iowrite32(dev, COHERENCE_REG, ACC_COH_NONE);

	iowrite32(dev, PT_ADDRESS_REG, (unsigned long long) ptable);
	iowrite32(dev, PT_NCHUNK_REG, NCHUNK);
	iowrite32(dev, PT_SHIFT_REG, CHUNK_SHIFT);
	iowrite32(dev, SRC_OFFSET_REG, 0);
	iowrite32(dev, DST_OFFSET_REG, SVHNMLP5_INBUF_SIZE);
	iowrite32(dev, SVHNMLP5_NINPUTS_REG, NINPUTS);

	// Configure point-to-point
	// no p2p

	// Flush for non-coherent DMA
	esp_flush(ACC_COH_NONE);

	// Start accelerators
	printf("  Start...\n");
	dev = &espdevs[0];
	iowrite32(dev, CMD_REG, CMD_MASK_START);

	// Wait for completion

	dev = &espdevs[0];
	done = 0;
	while (!done) {
	    done = ioread32(dev, STATUS_REG);
	    done &= STATUS_MASK_DONE;
	}

#ifndef __riscv
	printf("  Done device\n");
#else
	print_uart("  Done device\n");
#endif
	iowrite32(dev, CMD_REG, 0x0);

	printf("  Done\n");

	/* Validation */
	printf("  validating...\n");
	errors = validate_svhnmlp5(&mem[SIZE_IN]);
	if (errors)
		printf("  ... FAIL\n");
	else
		printf("  ... PASS\n");

	aligned_free(ptable);
	aligned_free(mem);

	return 0;
}

