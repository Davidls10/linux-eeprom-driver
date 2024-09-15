#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/err.h>

#define I2C_BUS_AVAILABLE 1
#define DEVICE_ADDRESS 0x50
#define DEVICE_MEM_SIZE 32768

dev_t device_number;
struct cdev at24c256_cdev;
struct class *class_at24c256;
struct device *device_at24c256;

static struct i2c_adapter *at24c256_i2c_adapter = NULL;
static struct i2c_client *at24c256_i2c_client = NULL;

static int at24c256_open(struct inode *inode, struct file *file);
static int at24c256_release(struct inode *inode, struct file *file);
static ssize_t at24c256_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t at24c256_write(struct file *filp, const char *buf, size_t len, loff_t *off);

static int sequential_read(unsigned char *buffer, unsigned char *read_data, unsigned int length);
static int page_write(unsigned char *buffer, unsigned int len);

struct file_operations at24c256_fops = {
	.owner = THIS_MODULE,
	.read = at24c256_read,
	.write = at24c256_write,
	.open = at24c256_open,
	.release = at24c256_release
};

static int at24c256_open(struct inode *inode, struct file *file) {
	return 0;
}

static int at24c256_release(struct inode *inode, struct file *file) {
	return 0;
}

static ssize_t at24c256_read(struct file *filp, char __user *buf, size_t len, loff_t *f_pos) {
	printk(KERN_INFO "Read requested for %zu bytes \n", len);
	if ((*f_pos + len) > DEVICE_MEM_SIZE - 1)
		len = (DEVICE_MEM_SIZE - 1) - *f_pos;

	unsigned char buf_sequential_read[2] = {0b00000001, 0b00001111};
	unsigned char sequential_read_data[len];
	sequential_read(buf_sequential_read, sequential_read_data, len);

	if (copy_to_user(buf, sequential_read_data, len)) {
		return -EFAULT;
	}
	*f_pos += len;

	return len;
}

static ssize_t at24c256_write(struct file *filp, const char *buf, size_t len, loff_t *off) {
	if ((*off + len) > DEVICE_MEM_SIZE - 1)
		len = DEVICE_MEM_SIZE - 1 - *off;

	unsigned char buf_page_write[len];
	unsigned char buf_page_write_addr[len];

	if (copy_from_user(buf_page_write, buf, len)) {
		return -EFAULT;
	}

	*off += len;

	buf_page_write_addr[0] = 0b00000001;
	buf_page_write_addr[1] = 0b00001111;
	for (int i = 0; i < len; i++) {
		buf_page_write_addr[i+2] = buf_page_write[i];
	}
	page_write(buf_page_write_addr, len+2);

	return len;
}

static int init_file(void) {
	if ((alloc_chrdev_region(&device_number, 0, 1, "at24c256_devices") < 0)) {
		printk(KERN_ALERT "Cannot allocate major number\n");
		return -1;
	}
	printk(KERN_INFO "Device number <major>:<minor> = %d:%d\n", MAJOR(device_number), MINOR(device_number));

	at24c256_cdev.owner = THIS_MODULE;
	cdev_init(&at24c256_cdev, &at24c256_fops);
	if ((cdev_add(&at24c256_cdev, device_number, 1) < 0)) {
		printk(KERN_ALERT "Cannot add the device to the system\n");
		goto r_class;
	}

	if (IS_ERR(class_at24c256 = class_create("at24c256_class"))) {
		printk(KERN_ALERT "Cannot create the struct class\n");
		goto r_class;
	}

	if (IS_ERR(device_at24c256 = device_create(class_at24c256, NULL, device_number, NULL, "at24c256"))) {
		printk(KERN_ALERT "Cannot create the device 1\n");
		goto r_device;
	}

	printk(KERN_INFO "Device driver insert done!");

	return 0;

	r_device:
		class_destroy(class_at24c256);
	r_class:
		unregister_chrdev_region(device_number, 1);
		return -1;
}

static void deinit_file(void) {
	device_destroy(class_at24c256, device_number);
	class_destroy(class_at24c256);
	cdev_del(&at24c256_cdev);
	unregister_chrdev_region(device_number, 1);
	printk(KERN_INFO "Module unloaded\n");
}

static int current_address_read(unsigned char *read_data) {
	int ret = i2c_master_recv(at24c256_i2c_client, read_data, 1);
	printk(KERN_ALERT "Current address read return: %d\n", ret);
	return ret;
}

static int sequential_read(unsigned char *buffer, unsigned char *read_data, unsigned int length) {
	struct i2c_msg msgs[2];
	msgs[0].addr = at24c256_i2c_client->addr;
	msgs[0].flags = 0;
	msgs[0].buf = buffer;
	msgs[0].len = 2;

	msgs[1].addr = at24c256_i2c_client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].buf = read_data;
	msgs[1].len = length;

	int ret = i2c_transfer(at24c256_i2c_client->adapter, msgs, 2);
	printk(KERN_ALERT "Sequential data return: %d\n", ret);
	return ret;
}

static int page_write(unsigned char *buffer, unsigned int len) {
	if (len > 66) {
		printk(KERN_ALERT "Write more than 64 data bytes in one burst not permitted!");
		return -1;
	}

	int ret = i2c_master_send(at24c256_i2c_client, buffer, len);
	printk(KERN_ALERT "Page write return: %d\n", ret);
	return ret;
}

static int random_read(unsigned char *buffer, unsigned char *read_data) {
	struct i2c_msg msgs[2];
	msgs[0].addr = at24c256_i2c_client->addr;
	msgs[0].flags = 0;
	msgs[0].buf = buffer;
	msgs[0].len = 2;

	msgs[1].addr = at24c256_i2c_client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].buf = read_data;
	msgs[1].len = 1;

	int ret = i2c_transfer(at24c256_i2c_client->adapter, msgs, 2);
	printk(KERN_ALERT "Random read return: %d\n", ret);
	return ret;
}

static int byte_write(unsigned char *buffer) {
	//struct i2c_msg msg;
	//msg.addr = at24c256_i2c_client->addr;
	//msg.buf = buffer;
	//msg.len = 3;
	//msg.flags = 0;
	//int ret = i2c_transfer(at24c256_i2c_client->adapter, &msg, 1);
	int ret = i2c_master_send(at24c256_i2c_client, buffer, 3);
	printk(KERN_ALERT "Byte write return: %d\n", ret);
	return ret;
}

static int at24c256_probe(struct i2c_client *client) {
	if (client->addr != DEVICE_ADDRESS) {
		printk(KERN_ALERT "AT24C256 - Wrong I2C address!\n");
		return -1;
	}

	printk(KERN_ALERT "AT24C256 probed");
    	return 0;
}

static void at24c256_remove(struct i2c_client *client) {
    	printk(KERN_ALERT "AT24C256 removed");
}

static const struct i2c_device_id at24c256_id[] = {
    	{ "AT24C256", 0 },
    	{ }
};
MODULE_DEVICE_TABLE(i2c, at24c256_id);

static struct i2c_driver at24c256_driver = {
    	.driver = {
            	.name = "AT24C256",
            	.owner = THIS_MODULE,
    	},
    	.probe = at24c256_probe,
    	.remove = at24c256_remove,
    	.id_table = at24c256_id
};

static struct i2c_board_info at24c256_board_info = {
    	I2C_BOARD_INFO("AT24C256", DEVICE_ADDRESS)
};

static int __init at24c256_driver_init(void) {
    	int ret = -1;

    	at24c256_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);

    	if (at24c256_i2c_adapter != NULL) {
            	at24c256_i2c_client = i2c_new_client_device(at24c256_i2c_adapter, &at24c256_board_info);
            	if (at24c256_i2c_client != NULL) {
                    	i2c_add_driver(&at24c256_driver);
                    	ret = 0;
            	}
            	i2c_put_adapter(at24c256_i2c_adapter);
    	}

	printk(KERN_ALERT "AT24C256 driver added");

	// read example
	unsigned char buf_random_read[2];
	buf_random_read[0] = 0b00000000;
	buf_random_read[1] = 0b00000001;
	unsigned char data;

	for (int i = 0; i < 10; i++) {
                random_read(buf_random_read, &data);
		printk("Received data: %d", data);
                for (int j = 0; j < 100; j++);
        }

	unsigned char buf_write_test[3];
	buf_write_test[0] = 0b00000000;
	buf_write_test[1] = 0b00000001;
    	buf_write_test[2] = 0b10101011;
	//byte_write(buf_write_test);

	// delay demonstration
	//for (int i = 0; i < 100; i++) {
	//	byte_write(buf_write_test);
	//	for (int j = 0; j < 1000; j++);
	//}

	// page write example
	unsigned char buf_page_write_test[66];
	buf_page_write_test[0] = 0b00000001;
	buf_page_write_test[1] = 0b00001111;
	for (int i = 0; i < 64; i++) {
		buf_page_write_test[i+2] = i;
	}
	//page_write(buf_page_write_test, 66);

	for (int i = 0; i < 100000000; i++); // delay

	// sequential read example
	unsigned char buf_sequential_read[2] = {0b00000001, 0b00000011};
	unsigned char sequential_read_data[64];
	sequential_read(buf_sequential_read, sequential_read_data, 64);
	for (int i = 0; i < 64; i++) {
		printk("Received sequential data[%d]: %d\n", i, sequential_read_data[i]);
	}

	// current address read example
	unsigned char buf_random_read_current[2] = {0b00000001, 0b00001111};
	random_read(buf_random_read_current, &data);
	printk("Received random read data for current address example: %d\n", data);
	current_address_read(&data);
	printk("Received current address data: %d\n", data);

	printk(KERN_INFO "Init file return: %d\n", init_file());

	return ret;
}

static void __exit at24c256_driver_exit(void) {
        i2c_unregister_device(at24c256_i2c_client);
        i2c_del_driver(&at24c256_driver);

	deinit_file();

        printk(KERN_ALERT "AT24C256 driver removed");
}

module_init(at24c256_driver_init);
module_exit(at24c256_driver_exit);

MODULE_LICENSE("GPL");
