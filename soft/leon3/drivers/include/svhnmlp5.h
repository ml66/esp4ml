#ifndef _SVHNMLP5_H_
#define _SVHNMLP5_H_

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

struct svhnmlp5_access {
	struct esp_access esp;
	unsigned tokens;
	unsigned batch;
	unsigned src_offset;
	unsigned dst_offset;
};

#define SVHNMLP5_IOC_ACCESS	_IOW ('S', 0, struct svhnmlp5_access)

#endif /* _SVHNMLP5_H_ */
