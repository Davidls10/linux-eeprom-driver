#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>

#define I2C_BUS_AVAILABLE 1
#define DEVICE_ADDRESS 0x50

#define AT24C256_WRITE_CMD 0b10100000
#define AT24C256_READ_CMD 0b10100001

static struct i2c_adapter *at24c256_i2c_adapter = NULL;
static struct i2c_client *at24c256_i2c_client = NULL;

static int byte_write(unsigned char *buf, unsigned int len) {
	int ret = i2c_transfer(at24c256_i2c_client, buf, 3);
	printk(KERN_ALERT "Write return: %d\n", ret);
	return ret;
}

static int byte_random_read(unsigned char *out_buf, unsigned int len) {
	int ret = i2c_master_recv(at24c256_i2c_client, out_buf, len);
	printk(KERN_ALERT "Read return: %d\n", ret);
	printk(KERN_ALERT "Outbuf[1]: %d", out_buf[1]);
	return ret;
}

static int at24c256_probe(struct i2c_client *client) {
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

	unsigned char buf_write_test[3] = {0};
	buf_write_test[0] = 0b00000000;
	buf_write_test[1] = 0b00000001;
    buf_write_test[2] = 0b10101011;
	byte_write(buf_write_test, 3);

    printk(KERN_ALERT "AT24C256 driver added");
    return ret;
}

static void __exit at24c256_driver_exit(void) {
        i2c_unregister_device(at24c256_i2c_client);
        i2c_del_driver(&at24c256_driver);

        printk(KERN_ALERT "AT24C256 driver removed");
}

module_init(at24c256_driver_init);
module_exit(at24c256_driver_exit);

MODULE_LICENSE("GPL");
