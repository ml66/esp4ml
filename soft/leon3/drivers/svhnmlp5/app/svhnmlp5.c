#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>

#include <my_stringify.h>
#include <test/test.h>
#include <test/time.h>
#include <svhnmlp5.h>

#define TOKENS 512
#define BATCH 256

typedef unsigned long long u64;

const unsigned out_offset = BATCH * TOKENS * sizeof(u64);
const unsigned size = 3 * out_offset;

struct accelerator_thread_info {
	int fd;
	struct svhnmlp5_access desc;
	unsigned long long hw_ns;
};

typedef struct accelerator_thread_info accelerator_thread_info_t;

void *accelerator_thread( void *ptr )
{
	accelerator_thread_info_t *info = (accelerator_thread_info_t *) ptr;
	struct timespec th_start;
	struct timespec th_end;
	int rc;

	gettime(&th_start);
	rc = ioctl(info->fd, SVHNMLP5_IOC_ACCESS, info->desc);
	gettime(&th_end);
	if(rc < 0) {
		perror("ioctl");
		exit(1);
	}
	info->hw_ns = ts_subtract(&th_start, &th_end);

	return NULL;
}

static void prepare_svhnmlp5(int *fd, contig_handle_t *mem, accelerator_thread_info_t *info, struct svhnmlp5_access *desc, const char *device_name, unsigned id)
{
		*fd = open(device_name, O_RDWR, 0);
		if (*fd < 0) {
			perror("open");
			exit(1);
		}

		desc->esp.contig = contig_to_khandle(*mem);
		desc->esp.run = true;
		desc->esp.coherence = ACC_COH_NONE;
		desc->tokens = TOKENS;
		switch(id) {
		case 0 :
			desc->batch = BATCH / 2;
			desc->src_offset = 0;
			desc->dst_offset = out_offset;
			break;
		case 1 :
			desc->batch = BATCH / 2;
			desc->src_offset = out_offset / 2;
			desc->dst_offset = 3 * out_offset / 2;
			break;
		default :
			desc->batch = BATCH;
			desc->src_offset = out_offset;
			desc->dst_offset = 2 * out_offset;
			break;
		}

		info->fd = *fd;
		info->desc = *desc;
}

static void init_buffer(contig_handle_t *hwbuf)
{
	int i, j;
	for (j = 0; j < BATCH; j++)
		for (i = 0; i < TOKENS; i++)
			contig_write64(0xFEED0BAC00000000L | (u64) i, *hwbuf, (j * TOKENS + i) * sizeof(u64));

	for (i = 0; i < 2 * BATCH * TOKENS; i++)
		contig_write64(0xFFFFFFFFFFFFFFFFL, *hwbuf, out_offset + i * sizeof(u64));
}

static void validate_buffer(contig_handle_t *hwbuf)
{
	int i, j;
	int err = 0;

	for (j = 0; j < BATCH; j++)
		for (i = 0; i < TOKENS; i++) {
			u64 token = contig_read64(*hwbuf, 2 * out_offset + (j * TOKENS + i) * sizeof(u64));
			if (token != (0xFEED0BAC00000000L | (u64) i)) {
				/* Only print first few errors to debug */
				if (err < 8)
					printf("      !! %d.%d: 0x%016llx !!\n", j, i, token);
				err++;
			}
		}

	if (!err)
		printf("      + Test PASSED\n");
	else
		printf("      + Test FAILED\n");
}

static void print_time_info(accelerator_thread_info_t *info, unsigned long long hw_ns, int nthreads)
{
	int i;
	for (i = 0; i < nthreads; i++)
		printf("      * ACC.%d time: %llu ns\n", i, info[i].hw_ns);
	printf("      Test time: %llu ns\n", hw_ns);
}


int main(int argc, char *argv[])
{
	int i;
	int rc = 0;
	int fd[3];
	contig_handle_t hwbuf;
	char device_name[60];

	struct timespec th_start;
	struct timespec th_end;

	pthread_t thread[3];
	accelerator_thread_info_t info[3];
	struct svhnmlp5_access desc[3];

	void *rp;

	printf("=== Point-to-point test w/ svhnmlp5 accelerator ===\n");

	printf("  - Config: {tokens = %d, batch = %d}\n", TOKENS, BATCH);

	printf("  - Allocate %u B\n", size);
	contig_alloc(size, &hwbuf);

	printf("  - Initialize input and clear output\n");
	init_buffer(&hwbuf);

	printf("\n");
	printf("     * Virtual memory layout *\n");
        printf("                  |_______________|\n");
        printf("    ACC0 src -->  |               |\n");
        printf("                  |    batch 0    |\n");
        printf("                  |_______________|\n");
        printf("                  |               |\n");
        printf("                         ...\n");
        printf("                  |_______________|\n");
        printf("                  |               |\n");
        printf("                  | batch B/2 - 1 |\n");
        printf("                  |_______________|\n");
        printf("    ACC1 src -->  |               |\n");
        printf("                  |   batch B/2   |\n");
        printf("                  |_______________|\n");
        printf("                  |               |\n");
        printf("                         ...\n");
        printf("                  |_______________|\n");
        printf("                  |               |\n");
        printf("                  |    batch B    |\n");
        printf("                  |_______________|\n");
        printf("    ACC2 src -->  |               | <-- ACC0 dst\n");
        printf("                  |    batch 0    |\n");
        printf("                  |_______________|\n");
        printf("                  |               |\n");
        printf("                         ...\n");
        printf("                  |_______________|\n");
        printf("                  |               |\n");
        printf("                  | batch B/2 - 1 |\n");
        printf("                  |_______________|\n");
        printf("                  |               | <-- ACC1 dst\n");
        printf("                  |   batch B/2   |\n");
        printf("                  |_______________|\n");
        printf("                  |               |\n");
        printf("                         ...\n");
        printf("                  |_______________|\n");
        printf("                  |               |\n");
        printf("                  |    batch B    |\n");
        printf("                  |_______________|\n");
        printf("    ACC2 dst -->  |               |\n");
        printf("                  |    batch 0    |\n");
        printf("                  |_______________|\n");
        printf("                  |               |\n");
        printf("                         ...\n");
        printf("                  |_______________|\n");
        printf("                  |               |\n");
        printf("                  |    batch B    |\n");
        printf("                  |_______________|\n");
	printf("\n");
	printf("\n");

	/* Simple DMA */
	printf("  --> Start simple DMA test\n");
	printf("  - Configure devices\n");
	for (i = 0; i < 3; i++) {
		sprintf(device_name, "/dev/svhnmlp5.%d", i);
		prepare_svhnmlp5(&fd[i], &hwbuf, &info[i], &desc[i], device_name, i);
	}
	printf("      MEM ==> ACC.0 ==> MEM\n");
	printf("      MEM ==> ACC.1 ==> MEM\n");
	printf("      MEM ==> ACC.2 ==> MEM\n");

	gettime(&th_start);
	for (i = 0; i < 2; i++) {
		rc = pthread_create(&thread[i], NULL, accelerator_thread, (void*) &info[i]);
		if(rc != 0) {
			perror("pthread_create");
			goto free_and_exit;
		}
	}
	for (i = 0; i < 2; i++) {
		rc = pthread_join(thread[i], NULL);
		if(rc != 0) {
			perror("pthread_join");
			goto free_and_exit;
		}
	}
	rp = accelerator_thread((void*) &info[i]);
	if(rp != NULL) {
		perror("accelerator_thread");
		goto free_and_exit;
	}
	gettime(&th_end);

	validate_buffer(&hwbuf);

	print_time_info(info, ts_subtract(&th_start, &th_end), 3);

free_and_exit:
	contig_free(hwbuf);
	return rc;

}


