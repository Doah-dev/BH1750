#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#define DRIVER_NAME "BH1750_driver"
#define CLASS_NAME "BH1750"
#define DEVICE_NAME "BH1750"

#define Power_Down 0x00
#define Power_On 0x01
#define Reset 0x07

#define Continuously_H_Resolution_Mode1 0x10
#define One_Time_H_Resolution_Mode1 0x20
#define Continuously_H_Resolution_Mode2 0x11
#define One_Time_H_Resolution_Mode2 0x21
#define Continuously_L_Resolution_Mode 0x13
#define One_Time_L_Resolution_Mode 0x23

#define IOCTL_SET_POWER_ON _IOW('k', 1, int)
#define IOCTL_SET_POWER_OFF _IOW('k', 2, int)
#define IOCTL_READ_Continuously_H_Mode1 _IOR('k', 3, int)
#define IOCTL_READ_One_H_Mode1 _IOR('k', 4, int)
#define IOCTL_READ_Continuously_H_Mode2 _IOR('k', 5, int)
#define IOCTL_READ_One_H_Mode2 _IOR('k', 6, int)
#define IOCTL_RESET _IOW('k', 7, int)
#define IOCTL_CHANGE_Measurement_Time _IOW('k', 8, int)
#define IOCTL_READ_Continuously_L_Mode _IOR('k', 9, int)
#define IOCTL_READ_One_L_Mode _IOR('k', 10, int)

static struct i2c_client *BH1750_client;
static struct class *BH1750_class = NULL;
static struct device *BH1750_device = NULL;
static int major_number;
static DEFINE_MUTEX(bh1750_mutex);

static int Change_Time(struct i2c_client *client, int value)
{
    uint8_t temp1 = (value >> 5) & 0x1F;
    uint8_t temp2 = value & 0x1F;

    temp1 |= 0x40;
    temp2 |= 0x60;

    if (i2c_smbus_write_byte(client, temp1) < 0) {
        pr_err("Failed to change measurement time (temp1)\n");
        return -EIO;
    }
    if (i2c_smbus_write_byte(client, temp2) < 0) {
        pr_err("Failed to change measurement time (temp2)\n");
        return -EIO;
    }

    pr_info("Succeed in changing measurement time\n");
    return 0;
}

static int BH1750_Power_Opr(struct i2c_client *client, int value)
{
    if (i2c_smbus_write_byte(client, value) < 0) {
        pr_err("Failed to set power operation (value: 0x%02x)\n", value);
        return -EIO;
    }

    pr_info("Succeed in setting power operation\n");
    return 0;
}

static int BH1750_Read_Mode(struct i2c_client *client, uint8_t mode, int *value, bool one_time)
{
    int ret;
    u8 buf[2];

    ret = i2c_smbus_write_byte(client, mode);
    if (ret < 0) {
        pr_err("Failed to write mode: 0x%02x\n", mode);
        return ret;
    }

    msleep(180);

    ret = i2c_smbus_read_i2c_block_data(client, 0x00, sizeof(buf), buf);
    if (ret != 2) {
        pr_err("Failed to read data from mode 0x%02x, ret=%d\n", mode, ret);
        return -EIO;
    }

    *value = (buf[0] << 8) | buf[1];

    if (one_time) {
        i2c_smbus_write_byte(client, Power_Down);
    }

    pr_info("Succeed in reading data from mode 0x%02x\n", mode);
    return 0;
}

static long BH1750_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int data = 0, ret = 0;

    mutex_lock(&bh1750_mutex);

    switch (cmd) {
        case IOCTL_READ_Continuously_H_Mode1:
            ret = BH1750_Read_Mode(BH1750_client, Continuously_H_Resolution_Mode1, &data, false);
            break;
        case IOCTL_READ_One_H_Mode1:
            ret = BH1750_Read_Mode(BH1750_client, One_Time_H_Resolution_Mode1, &data, true);
            break;
        case IOCTL_READ_Continuously_H_Mode2:
            ret = BH1750_Read_Mode(BH1750_client, Continuously_H_Resolution_Mode2, &data, false);
            break;
        case IOCTL_READ_One_H_Mode2:
            ret = BH1750_Read_Mode(BH1750_client, One_Time_H_Resolution_Mode2, &data, true);
            break;
        case IOCTL_READ_Continuously_L_Mode:
            ret = BH1750_Read_Mode(BH1750_client, Continuously_L_Resolution_Mode, &data, false);
            break;
        case IOCTL_READ_One_L_Mode:
            ret = BH1750_Read_Mode(BH1750_client, One_Time_L_Resolution_Mode, &data, true);
            break;
        case IOCTL_SET_POWER_ON:
            ret = BH1750_Power_Opr(BH1750_client, Power_On);
            break;
        case IOCTL_SET_POWER_OFF:
            ret = BH1750_Power_Opr(BH1750_client, Power_Down);
            break;
        case IOCTL_RESET:
            ret = BH1750_Power_Opr(BH1750_client, Reset);
            break;
        case IOCTL_CHANGE_Measurement_Time:
            if (copy_from_user(&data, (int __user *)arg, sizeof(int)) != 0) {
                ret = -EFAULT;
                break;
            }
            ret = Change_Time(BH1750_client, data);
            break;
        default:
            ret = -EINVAL;
    }

    if (ret == 0 && _IOC_DIR(cmd) & _IOC_READ) {
        if (copy_to_user((int __user *)arg, &data, sizeof(int)) != 0)
            ret = -EFAULT;
    }

    mutex_unlock(&bh1750_mutex);
    return ret;
}

static int BH1750_open(struct inode *inodep, struct file *filep)
{
    pr_info("BH1750 device opened\n");
    return 0;
}

static int BH1750_release(struct inode *inodep, struct file *filep)
{
    pr_info("BH1750 device closed\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = BH1750_open,
    .unlocked_ioctl = BH1750_ioctl,
    .release = BH1750_release,
};

static int BH1750_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    BH1750_client = client;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE | I2C_FUNC_SMBUS_I2C_BLOCK)) {
        pr_err("I2C adapter does not support required functionality\n");
        return -EIO;
    }

    if (i2c_smbus_write_byte(client, Power_On) < 0) {
        pr_err("Failed to power on the device\n");
        return -EIO;
    }

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        pr_err("Failed to register major number\n");
        return major_number;
    }

    BH1750_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(BH1750_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(BH1750_class);
    }

    BH1750_device = device_create(BH1750_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(BH1750_device)) {
        class_destroy(BH1750_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        return PTR_ERR(BH1750_device);
    }

    mutex_init(&bh1750_mutex);
    pr_info("BH1750 device initialized successfully\n");
    return 0;
}

static void BH1750_remove(struct i2c_client *client)
{
    device_destroy(BH1750_class, MKDEV(major_number, 0));
    class_unregister(BH1750_class);
    class_destroy(BH1750_class);
    unregister_chrdev(major_number, DEVICE_NAME);
    mutex_destroy(&bh1750_mutex);
    pr_info("BH1750 device removed\n");
}


static const struct of_device_id BH1750_of_match[] = {
    { .compatible = "rohm,BH1750", },
    { },
};
MODULE_DEVICE_TABLE(of, BH1750_of_match);

static struct i2c_driver BH1750_driver = {
    .driver = {
        .name   = DRIVER_NAME,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(BH1750_of_match),
    },
    .probe      = BH1750_probe,
    .remove     = BH1750_remove,
};

static int __init BH1750_init(void)
{
    return i2c_add_driver(&BH1750_driver);
}

static void __exit BH1750_exit(void)
{
    i2c_del_driver(&BH1750_driver);
}

module_init(BH1750_init);
module_exit(BH1750_exit);

MODULE_AUTHOR("Nhat hao");
MODULE_DESCRIPTION("A driver for BH1750 light sensor");
MODULE_LICENSE("GPL");
