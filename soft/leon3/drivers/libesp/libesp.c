/*
 * Copyright (c) 2011-2019 Columbia University, System Level Design Group
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libesp.h"

static contig_handle_t *contig;
static pthread_t *thread;

void *accelerator_thread( void *ptr )
{
	esp_thread_info_t *info = (esp_thread_info_t *) ptr;
	struct timespec th_start;
	struct timespec th_end;
	int rc = 0;

	gettime(&th_start);
	switch (info->type) {
	case adder :
		rc = ioctl(info->fd, ADDER_IOC_ACCESS, info->desc.adder_desc);
		break;
	case CounterAccelerator :
		rc = ioctl(info->fd, COUNTERACCELERATOR_IOC_ACCESS, info->desc.CounterAccelerator_desc);
		break;
	case dummy :
		rc = ioctl(info->fd, DUMMY_IOC_ACCESS, info->desc.dummy_desc);
		break;
	case sort :
		rc = ioctl(info->fd, SORT_IOC_ACCESS, info->desc.sort_desc);
		break;
	case spmv :
		rc = ioctl(info->fd, SPMV_IOC_ACCESS, info->desc.spmv_desc);
		break;
	case svhnmlp :
		rc = ioctl(info->fd, SVHNMLP_IOC_ACCESS, info->desc.svhnmlp_desc);
		break;
	case synth :
		rc = ioctl(info->fd, SYNTH_IOC_ACCESS, info->desc.synth_desc);
		break;
	case visionchip :
		rc = ioctl(info->fd, VISIONCHIP_IOC_ACCESS, info->desc.visionchip_desc);
		break;
	case vitbfly2 :
		rc = ioctl(info->fd, VITBFLY2_IOC_ACCESS, info->desc.vitbfly2_desc);
		break;
	}
	gettime(&th_end);
	if (rc < 0) {
		perror("ioctl");
	}
	info->hw_ns = ts_subtract(&th_start, &th_end);

	return NULL;
}

void esp_alloc(contig_handle_t *handle, void *swbuf, size_t size, size_t in_size)
{
	contig = handle;
	contig_alloc(size, contig);
	contig_copy_to(*contig, 0, swbuf, in_size);
}

static void esp_prepare(struct esp_access *esp)
{
	esp->contig = contig_to_khandle(*contig);
	esp->run = true;
}

static void esp_config(esp_thread_info_t cfg[], unsigned nacc)
{
	int i;
	for (i = 0; i < nacc; i++) {
		esp_thread_info_t *info = &cfg[i];

		if (!info->run)
			continue;

		switch (info->type) {
		case adder:
			esp_prepare(&info->desc.adder_desc.esp);
			break;
		case CounterAccelerator:
			esp_prepare(&info->desc.CounterAccelerator_desc.esp);
			break;
		case dummy:
			esp_prepare(&info->desc.dummy_desc.esp);
			break;
		case sort:
			esp_prepare(&info->desc.sort_desc.esp);
			break;
		case spmv:
			esp_prepare(&info->desc.spmv_desc.esp);
			break;
		case svhnmlp:
			esp_prepare(&info->desc.svhnmlp_desc.esp);
			break;
		case synth:
			esp_prepare(&info->desc.synth_desc.esp);
			break;
		case visionchip:
			esp_prepare(&info->desc.visionchip_desc.esp);
			break;
		case vitbfly2:
			esp_prepare(&info->desc.vitbfly2_desc.esp);
			break;
		default :
			contig_free(*contig);
			die("Error: accelerator type specified for accelerator %s not supported\n", info->devname);
			break;
		}
	}
}

static void print_time_info(esp_thread_info_t info[], unsigned long long hw_ns, int nthreads)
{
	int i;

	for (i = 0; i < nthreads; i++)
		if (info->run)
			printf("%s time: %llu ns\n", info[i].devname, info[i].hw_ns);

	printf("Test time: %llu ns\n", hw_ns);
}

void esp_run(esp_thread_info_t cfg[], unsigned nacc)
{
	int i;
	struct timespec th_start;
	struct timespec th_end;
	thread = malloc(nacc * sizeof(pthread_t));
	int rc = 0;

	esp_config(cfg, nacc);

	for (i = 0; i < nacc; i++) {
		esp_thread_info_t *info = &cfg[i];
		char path[70];
		const char *prefix = "/dev/";

		if (strlen(info->devname) > 64) {
			contig_free(*contig);
			die("Error: device name %s exceeds maximum length of 64 characters\n", info->devname);
		}

		sprintf(path, "%s%s", prefix, info->devname);

		info->fd = open(path, O_RDWR, 0);
		if (info->fd < 0) {
			contig_free(*contig);
			die_errno("fopen failed\n");
		}
	}

	gettime(&th_start);
	for (i = 0; i < nacc; i++) {
		esp_thread_info_t *info = &cfg[i];

		if (!info->run)
			continue;

		rc = pthread_create(&thread[i], NULL, accelerator_thread, (void*) info);
		if(rc != 0) {
			perror("pthread_create");
		}
	}
	for (i = 0; i < nacc; i++) {
		esp_thread_info_t *info = &cfg[i];

		if (!info->run)
			continue;

		rc = pthread_join(thread[i], NULL);
		if(rc != 0) {
			perror("pthread_join");
		}
		close(info->fd);
	}
	gettime(&th_end);
	print_time_info(cfg, ts_subtract(&th_start, &th_end), nacc);

	free(thread);
}


void esp_dump(void *swbuf, size_t size)
{
	contig_copy_from(swbuf, *contig, 0, size);
}

void esp_cleanup()
{
	contig_free(*contig);
}
