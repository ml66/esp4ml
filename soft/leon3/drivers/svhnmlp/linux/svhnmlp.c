#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "svhnmlp.h"

#define DRV_NAME "svhnmlp"

#define SVHNMLP_NINPUTS_REG	0x40

struct svhnmlp_device {
	struct esp_device esp;
};

static struct esp_driver svhnmlp_driver;

static struct of_device_id svhnmlp_device_ids[] = {
	{
		.name = "SLD_SVHNMLP",
	},
	{
		.name = "eb_043",
	},
	{
		.compatible = "sld,svhnmlp",
	},
	{ },
};

static int svhnmlp_devs;

static inline struct svhnmlp_device *to_svhnmlp(struct esp_device *esp)
{
	return container_of(esp, struct svhnmlp_device, esp);
}

static void svhnmlp_prep_xfer(struct esp_device *esp, void *arg)
{
	struct svhnmlp_access *a = arg;

	iowrite32be(a->ninputs, esp->iomem + SVHNMLP_NINPUTS_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);
}

static bool svhnmlp_xfer_input_ok(struct esp_device *esp, void *arg)
{
	return true;
}

static int svhnmlp_probe(struct platform_device *pdev)
{
	struct svhnmlp_device *svhnmlp;
	struct esp_device *esp;
	int rc;

	svhnmlp = kzalloc(sizeof(*svhnmlp), GFP_KERNEL);
	if (svhnmlp == NULL)
		return -ENOMEM;
	esp = &svhnmlp->esp;
	esp->module = THIS_MODULE;
	esp->number = svhnmlp_devs;
	esp->driver = &svhnmlp_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	svhnmlp_devs++;
	return 0;
 err:
	kfree(svhnmlp);
	return rc;
}

static int __exit svhnmlp_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct svhnmlp_device *svhnmlp = to_svhnmlp(esp);

	esp_device_unregister(esp);
	kfree(svhnmlp);
	return 0;
}

static struct esp_driver svhnmlp_driver = {
	.plat = {
		.probe		= svhnmlp_probe,
		.remove		= svhnmlp_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = svhnmlp_device_ids,
		},
	},
	.xfer_input_ok	= svhnmlp_xfer_input_ok,
	.prep_xfer	= svhnmlp_prep_xfer,
	.ioctl_cm	= SVHNMLP_IOC_ACCESS,
	.arg_size	= sizeof(struct svhnmlp_access),
};

static int __init svhnmlp_init(void)
{
	return esp_driver_register(&svhnmlp_driver);
}

static void __exit svhnmlp_exit(void)
{
	esp_driver_unregister(&svhnmlp_driver);
}

module_init(svhnmlp_init)
module_exit(svhnmlp_exit)

MODULE_DEVICE_TABLE(of, svhnmlp_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("svhnmlp driver");
