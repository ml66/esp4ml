#include <linux/of_device.h>
#include <linux/mm.h>

#include <asm/io.h>

#include <esp_accelerator.h>
#include <esp.h>

#include "autoenc.h"

#define DRV_NAME "autoenc"

#define AUTOENC_NINPUTS_REG	0x40

struct autoenc_device {
	struct esp_device esp;
};

static struct esp_driver autoenc_driver;

static struct of_device_id autoenc_device_ids[] = {
	{
		.name = "SLD_AUTOENC",
	},
	{
		.name = "eb_043",
	},
	{
		.compatible = "sld,autoenc",
	},
	{ },
};

static int autoenc_devs;

static inline struct autoenc_device *to_autoenc(struct esp_device *esp)
{
	return container_of(esp, struct autoenc_device, esp);
}

static void autoenc_prep_xfer(struct esp_device *esp, void *arg)
{
	struct autoenc_access *a = arg;

	iowrite32be(a->ninputs, esp->iomem + AUTOENC_NINPUTS_REG);
	iowrite32be(a->src_offset, esp->iomem + SRC_OFFSET_REG);
	iowrite32be(a->dst_offset, esp->iomem + DST_OFFSET_REG);
}

static bool autoenc_xfer_input_ok(struct esp_device *esp, void *arg)
{
	return true;
}

static int autoenc_probe(struct platform_device *pdev)
{
	struct autoenc_device *autoenc;
	struct esp_device *esp;
	int rc;

	autoenc = kzalloc(sizeof(*autoenc), GFP_KERNEL);
	if (autoenc == NULL)
		return -ENOMEM;
	esp = &autoenc->esp;
	esp->module = THIS_MODULE;
	esp->number = autoenc_devs;
	esp->driver = &autoenc_driver;
	rc = esp_device_register(esp, pdev);
	if (rc)
		goto err;

	autoenc_devs++;
	return 0;
 err:
	kfree(autoenc);
	return rc;
}

static int __exit autoenc_remove(struct platform_device *pdev)
{
	struct esp_device *esp = platform_get_drvdata(pdev);
	struct autoenc_device *autoenc = to_autoenc(esp);

	esp_device_unregister(esp);
	kfree(autoenc);
	return 0;
}

static struct esp_driver autoenc_driver = {
	.plat = {
		.probe		= autoenc_probe,
		.remove		= autoenc_remove,
		.driver		= {
			.name = DRV_NAME,
			.owner = THIS_MODULE,
			.of_match_table = autoenc_device_ids,
		},
	},
	.xfer_input_ok	= autoenc_xfer_input_ok,
	.prep_xfer	= autoenc_prep_xfer,
	.ioctl_cm	= AUTOENC_IOC_ACCESS,
	.arg_size	= sizeof(struct autoenc_access),
};

static int __init autoenc_init(void)
{
	return esp_driver_register(&autoenc_driver);
}

static void __exit autoenc_exit(void)
{
	esp_driver_unregister(&autoenc_driver);
}

module_init(autoenc_init)
module_exit(autoenc_exit)

MODULE_DEVICE_TABLE(of, autoenc_device_ids);

MODULE_AUTHOR("Emilio G. Cota <cota@braap.org>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("autoenc driver");
