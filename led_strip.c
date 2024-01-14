#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>

#define MAX_DEVICES 2
#define DEVICE_NAME "led_strip"

dev_t device_number;
static struct class *led_strip_class;
static struct cdev led_strip_cdev[MAX_DEVICES];

struct led_strip_driver_priv {
	/**
	 * @cdev: reference to parent character device
	 */
	struct cdev *cdev;
	/**
	 * @strip_no: Which strip this is (corresponds to device minor no)
	 */
	int strip_no;
};

static int led_strip_open(struct inode *inode, struct file *file)
{
	struct led_strip_driver_priv *ctx;

	file->private_data = kzalloc(sizeof(struct led_strip_driver_priv), GFP_KERNEL);
	if (!file->private_data)
		return -EINVAL;
	ctx = file->private_data;
	ctx->cdev = inode->i_cdev;
	ctx->strip_no = MINOR(ctx->cdev->dev);

  pr_info("Opened led strip %d\n", ctx->strip_no);

  return 0;
}

static int led_strip_release(struct inode *inode, struct file *file)
{
  int ret = 0;
	struct led_strip_driver_priv *ctx = file->private_data;

	if (ctx) {
		pr_info("Closing led_strip %d\n", ctx->strip_no);
		kfree(ctx);
		file->private_data = NULL;
	}
	else {
		ret = -ENODEV;
	}

	return ret;
};

ssize_t led_strip_read(struct file *file, char __user *user_buffer,
		       size_t count, loff_t *offset)
{
	pr_info("%s\n", __func__);
	return 0;
}

ssize_t led_strip_write(struct file *file, const char __user *user_buffer,
			size_t count, loff_t *offset)
{
	pr_info("%s\n", __func__);
	return count;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = led_strip_open,
	.release = led_strip_release,
	.read = led_strip_read,
	.write = led_strip_write,
};

static int __init ledstrip_init(void)
{
	int ret;

	pr_info("%s: Initialization\n", __func__);
 
	ret = alloc_chrdev_region(&device_number, 0, MAX_DEVICES, DEVICE_NAME);	
	if (!ret) {
		int major = MAJOR(device_number);
		dev_t my_device;
		int i = 0;

		led_strip_class = class_create(THIS_MODULE, "led_strip_class");
		for(i = 0; i < MAX_DEVICES; ++i) {
			my_device = MKDEV(major, i);
			cdev_init(&led_strip_cdev[i], &fops);
			pr_info("%s alloc'd cdev %p\n", __func__, &led_strip_cdev[i]);
			ret = cdev_add(&led_strip_cdev[i], my_device, 1);
			if (ret){
				pr_err("%s: Failed in adding cdev to subsystem %d\n", __func__, ret);
			}
			else {
				device_create(led_strip_class, NULL, my_device, NULL, DEVICE_NAME "%d", i);
				pr_info("%s: Successfully created device %s%d\n", __func__, DEVICE_NAME, i);
			}
		}
	}
	else {
		pr_err("%s: Failed to allocate device number (%d)\n", __func__, ret);
	}

	return ret;
}

static void __exit ledstrip_cleanup(void)
{
	int i = 0;
	int major = MAJOR(device_number);
	dev_t my_device;

	pr_info("Cleaning up module %s\n", DEVICE_NAME);

	for (i = 0; i < MAX_DEVICES; i++) {
		my_device = MKDEV(major, i);
		cdev_del(&led_strip_cdev[i]);
		device_destroy(led_strip_class, my_device);
	}
	class_destroy(led_strip_class);
	unregister_chrdev_region(device_number, MAX_DEVICES);
}

module_init(ledstrip_init);
module_exit(ledstrip_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Taylor Nelms <taylornelms15@gmail.com>");
MODULE_DESCRIPTION("A module to set up output LED devices for local control by Hyperion");
