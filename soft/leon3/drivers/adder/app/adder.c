#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include <my_stringify.h>
#include <test/test.h>
#include <test/time.h>
#include <adder.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#define DEVNAME "/dev/adder.0"
#define NAME "adder"

// Default command line arguments
#define DEFAULT_SIZE 128

static const char usage_str[] =
	"usage: ./adder.exe coherence infile [size] [-v]\n"
	"  coherence: non-coh-dma|llc-coh-dma|coh-dma|coh\n"
	"  infile : input file name (includes the path)\n"
	"\n"
	"Optional arguments:.\n"
	"  size : size of the two input streams and of the output. Default: 128\n"
	"\n"
	"The remaining option is only optional for 'test':\n"
        "  -v: enable verbose output for output-to-gold comparison\n";

struct adder_test {
	struct test_info info;
	struct adder_access desc;
	char *infile;
	unsigned int size;
	unsigned int *hbuf;
	unsigned int *sbuf;
	bool verbose;
};

static inline struct adder_test *to_adder(struct test_info *info)
{
	return container_of(info, struct adder_test, info);
}

static int check_gold (unsigned int *gold, unsigned int *array, unsigned len, bool verbose)
{
	int i;
	int rtn = 0;
	for (i = 0; i < len; i++) {
		if (((int) array[i]) != gold[i]) {
			if (verbose)
				printf("A[%d]: array=%d; gold=%d\n",
				       i, array[i], gold[i]);
			rtn++;
		}
	}

	return rtn;
}

static void init_buf (struct adder_test *t)
{
        //  ========================  ^
        //  |  in/out image (int)  |  | img_size (in bytes)
        //  ========================  v

        printf("init buffers\n");

        int i = 0;

	// store input in accelerator buffer
	for (i = 0; i < t->size; i++) {
	    t->hbuf[i*2] = i*2;
	    t->hbuf[i*2+1] = i*2+1;
	    t->sbuf[i*2] = i*2;
	    t->sbuf[i*2+1] = i*2+1;
	    t->sbuf[t->size * 2 + i] = i * 2 + i * 2 + 1;
	}

        /* fclose(fd); */
}

static inline size_t adder_size(struct adder_test *t)
{
	return t->size * 3 * sizeof(unsigned int);
}

static void adder_alloc_buf(struct test_info *info)
{
	struct adder_test *t = to_adder(info);

	t->hbuf = malloc0_check(adder_size(t));
	if (!strcmp(info->cmd, "test")) {
		t->sbuf = malloc0_check(adder_size(t));
	}
}

static void adder_alloc_contig(struct test_info *info)
{
	struct adder_test *t = to_adder(info);

	printf("HW buf size: %zu MB\n", adder_size(t) / 1000000);
	if (contig_alloc(adder_size(t), &info->contig))
		die_errno(__func__);
}

static void adder_init_bufs(struct test_info *info)
{
	struct adder_test *t = to_adder(info);

	init_buf(t);
	contig_copy_to(info->contig, 0, t->hbuf, adder_size(t));
}

static void adder_set_access(struct test_info *info)
{
	struct adder_test *t = to_adder(info);

	t->desc.size = t->size;
}

static void adder_comp(struct test_info *info)
{
}

static bool adder_diff_ok(struct test_info *info)
{
	struct adder_test *t = to_adder(info);
	int total_err = 0;

	contig_copy_from(t->hbuf, info->contig, 0, adder_size(t));

	int err;

	err = check_gold(t->sbuf, t->hbuf, t->size * 3, t->verbose);
	if (err)
		printf("%d mismatches\n", err);
	else
		printf("NICE! %d mismatches\n", err);

	total_err += err;

	if (total_err)
		printf("%d mismatches in total\n", total_err);
	return !total_err;
}

static struct adder_test adder_test = {
	.info = {
		.name		= NAME,
		.devname	= DEVNAME,
		.alloc_buf	= adder_alloc_buf,
		.alloc_contig	= adder_alloc_contig,
		.init_bufs	= adder_init_bufs,
		.set_access	= adder_set_access,
		.comp		= adder_comp,
		.diff_ok	= adder_diff_ok,
		.esp		= &adder_test.desc.esp,
		.cm		= ADDER_IOC_ACCESS,
	},
};

static void NORETURN usage(void)
{
	fprintf(stderr, "%s", usage_str);
	exit(1);
}

int main(int argc, char *argv[])
{
	if (argc < 3 || argc > 5) {
		usage();

	} else {
		printf("\nCommand line arguments received:\n");
		printf("\tcoherence: %s\n", argv[1]);

		adder_test.infile = argv[2];
		printf("\tinfile: %s\n", adder_test.infile);

		if (argc == 4) {
		    if (strcmp(argv[3], "-v")) {
			adder_test.size = strtol(argv[3], NULL, 10);
			printf("\tnsize: %u\n", adder_test.size);
		    } else {
			adder_test.size = DEFAULT_SIZE;
			adder_test.verbose = true;
			printf("\tverbose enabled\n");
		    }
		} else {
			adder_test.size = DEFAULT_SIZE;
		}

		if (argc == 5) {
			if (strcmp(argv[4], "-v")) {
				usage();
			} else {
				adder_test.verbose = true;
				printf("\tverbose enabled\n");
			}
		}
		printf("\n");
	}

	return test_main(&adder_test.info, argv[1], "test");
}
