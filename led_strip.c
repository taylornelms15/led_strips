#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/parser.h>
#include <linux/printk.h>

#include "led_strip.h"
#include "led_control.h"

#define MAX_DEVICES 2
#define DEVICE_NAME "led_strip"

dev_t device_number;
static struct class *led_strip_class;

struct led_strip_device {
	struct device *dev;
	struct cdev cdev;
};

#define cdev_to_dev(cdev_ptr) (container_of(cdev_ptr, struct led_strip_device, cdev)->dev)

static struct led_strip_device led_strip_devices[MAX_DEVICES];

//static struct cdev led_strip_cdev[MAX_DEVICES];

/* LED Control */

/**
 * parse_control_string() - Parses a control string as sent by Hyperion
 * 
 * Assumes the string contains the following substring:
 * "[{r0,g0,b0}{r1,g1,b1}{r2,g2,b2}...{rn,gn,bn}]"
 * where rx, gx, bx are decimal values between 0 and 255 representing
 * brightness components for the leds
 *
 * @Return: number of LED's parsed on success, or negative value on error
 */
static int parse_control_string(struct led_strip_priv *ctx, char *buf, size_t count)
{
	char* head = buf;
	char next = '\0';
	int led_count = 0;

	pr_info("%s: <%s> count %ld\n", __func__, buf, count);

	/* parse until first '[' */
	do {
		next = *head++;
	} while (next != '[');
	/* head is at the first char after [ */

	next = *head;

	/* Parse some number of LED strings */
	while (next != ']' && led_count < MAX_LEDS_PER_DEVICE) {
		if (next == '{') {
			char *num_token;
			substring_t num_substring;
			unsigned int match_value;
			int match_result = 0;

			head++;

			/* Red */
			num_token = strsep(&head, ",");
			num_substring.from = num_token;
			num_substring.to = head - 1;
			match_result = match_uint(&num_substring, &match_value);
			ctx->values_to_write[led_count * 3] = match_value;
			if (match_result)
				pr_err("%s: Error on red num_token %s, led_count %d, err %d\n",
				       __func__, num_token, led_count, match_result);
			
			/* Green */
			num_token = strsep(&head, ",");
			num_substring.from = num_token;
			num_substring.to = head - 1;
			match_result = match_uint(&num_substring, &match_value);
			ctx->values_to_write[led_count * 3 + 1] = match_value;
			if (match_result)
				pr_err("%s: Error on grn num_token %s, led_count %d, err %d\n",
				       __func__, num_token, led_count, match_result);
				
			/* Blue */
			num_token = strsep(&head, "}");
			num_substring.from = num_token;
			num_substring.to = head - 1;
			match_result = match_uint(&num_substring, &match_value);
			ctx->values_to_write[led_count * 3 + 2] = match_value;
			if (match_result)
				pr_err("%s: Error on blu num_token %s, led_count %d, err %d\n",
				       __func__, num_token, led_count, match_result);

			next = *head;
			++led_count;
		}
		else {
			if (head == (buf + count)) {
				pr_err("%s: Error, went past count searching for ] or {\n", __func__);
				return -EINVAL;
			}
			next = *head++;
		}
	}/* end:while */

	return led_count;
}

/* Class Operations */

static int led_strip_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0222);
	return 0;
}

/* File Operations */

static int led_strip_open(struct inode *inode, struct file *file)
{
	struct led_strip_priv *ctx;
	int ret = 0;

	file->private_data = kzalloc(sizeof(struct led_strip_priv), GFP_KERNEL);
	if (!file->private_data)
		return -EINVAL;
	ctx = file->private_data;
	ctx->cdev = inode->i_cdev;
	ctx->dev = cdev_to_dev(ctx->cdev);
	if (MINOR(ctx->cdev->dev) == 0)
		ctx->strip_no = LED_0;
	else
		ctx->strip_no = LED_1;

	ret = prepare_output_gpio(ctx);

	pr_info("Opened led strip %d, ret %d\n", ctx->strip_no, ret);

	return 0;
}

static int led_strip_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	struct led_strip_priv *ctx = file->private_data;


	if (ctx) {
		pr_info("Closing led_strip %d\n", ctx->strip_no);
		ret = release_output_gpio(ctx);
		kfree(ctx);
		file->private_data = NULL;
	}
	else {
		ret = -ENODEV;
	}

	return ret;
};

ssize_t led_strip_write(struct file *file, const char __user *user_buffer,
			size_t count, loff_t *offset)
{
	struct led_strip_priv *ctx = file->private_data;
	int ret = 0;
	char *tmp_buffer = kzalloc(count + 1, GFP_KERNEL);

	if (!tmp_buffer)
		return 0;
	
	strscpy(tmp_buffer, user_buffer, count);

	ret = parse_control_string(ctx, tmp_buffer, count);
	
	/* Debug: print parsed led values */
	if (ret <= 0)
		goto out_write;

	output_led_values(ctx, ret);

out_write:
	kfree(tmp_buffer);
	return count;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = led_strip_open,
	.release = led_strip_release,
	.write = led_strip_write,
};

/* Device Operations */

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
		led_strip_class->dev_uevent = led_strip_uevent;
		for(i = 0; i < MAX_DEVICES; ++i) {
			my_device = MKDEV(major, i);
			cdev_init(&led_strip_devices[i].cdev, &fops);
			pr_info("%s alloc'd cdev %p\n", __func__, &led_strip_devices[i].cdev);
			ret = cdev_add(&led_strip_devices[i].cdev, my_device, 1);
			if (ret){
				pr_err("%s: Failed in adding cdev to subsystem %d\n", __func__, ret);
			}
			else {
				led_strip_devices[i].dev =
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
		cdev_del(&led_strip_devices[i].cdev);
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
