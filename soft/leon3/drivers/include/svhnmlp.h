#ifndef _SVHNMLP_H_
#define _SVHNMLP_H_

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

struct svhnmlp_access {
	struct esp_access esp;
	unsigned tokens;
	unsigned batch;
	unsigned src_offset;
	unsigned dst_offset;
};

#define SVHNMLP_IOC_ACCESS	_IOW ('S', 0, struct svhnmlp_access)

#endif /* _SVHNMLP_H_ */
