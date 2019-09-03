#ifndef _ADDER_H_
#define _ADDER_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
#ifndef __user
#define __user
#endif
#endif /* __KERNEL__ */

#include <esp.h>
#include <esp_accelerator.h>

#define MAX_SIZE (1 << 30)

struct adder_access {
	struct esp_access esp;
	// Size of input and output
	unsigned int size;
};

#define ADDER_IOC_ACCESS	_IOW ('S', 0, struct adder_access)

#endif /* _ADDER_H_ */
