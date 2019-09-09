#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "svhnmlp5.h"

#define DRV_NAME "svhnmlp5"

#define SVHNMLP5_NINPUTS_REG	0x40

struct svhnmlp5_device {
	struct esp_device esp;
};

static struct esp_driver svhnmlp5_driver;

static struct of_device_id svhnmlp5_device_ids[] = {
	{
		.name = "SLD_SVHNMLP5",
	},
	{
		.name = "eb_045",
	},
	{
		.compatible = "sld,svhnmlp5",
	},
	{ },
};

static int svhnmlp5_devs;

static inline struct svhnmlp5_device *to_svhnmlp5(struct esp_device *esp)
{
	return container_of(esp, struct svhnmlp5_device, esp);
}

static void svhnmlp5_prep_xfer(struct esp_device *esp, void *arg)
{
	struct svhnmlp5_access *a = arg;

	iowrite32be(a->ninputs, esp->iomem + SVHNMLP5_NINPUTS_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);
}

static bool svhnmlp5_xfer_input_ok(struct esp_device *esp, void *arg)
{
	return true;
}

static int svhnmlp5_probe(struct platform_device *pdev)
{
	struct svhnmlp5_device *svhnmlp5;
	struct esp_device *esp;
	int rc;

	svhnmlp5 = kzalloc(sizeof(*svhnmlp5), GFP_KERNEL);
	if (svhnmlp5 == NULL)
		return -ENOMEM;
	esp = &svhnmlp5->esp;
	esp->module = THIS_MODULE;
	esp->number = svhnmlp5_devs;
	esp->driver = &svhnmlp5_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	svhnmlp5_devs++;
	return 0;
 err:
	kfree(svhnmlp5);
	return rc;
}

static int __exit svhnmlp5_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct svhnmlp5_device *svhnmlp5 = to_svhnmlp5(esp);

	esp_device_unregister(esp);
	kfree(svhnmlp5);
	return 0;
}

static struct esp_driver svhnmlp5_driver = {
	.plat = {
		.probe		= svhnmlp5_probe,
		.remove		= svhnmlp5_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = svhnmlp5_device_ids,
		},
	},
	.xfer_input_ok	= svhnmlp5_xfer_input_ok,
	.prep_xfer	= svhnmlp5_prep_xfer,
	.ioctl_cm	= SVHNMLP5_IOC_ACCESS,
	.arg_size	= sizeof(struct svhnmlp5_access),
};

static int __init svhnmlp5_init(void)
{
	return esp_driver_register(&svhnmlp5_driver);
}

static void __exit svhnmlp5_exit(void)
{
	esp_driver_unregister(&svhnmlp5_driver);
}

module_init(svhnmlp5_init)
module_exit(svhnmlp5_exit)

MODULE_DEVICE_TABLE(of, svhnmlp5_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("svhnmlp5 driver");
