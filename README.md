# BH1750 Driver
# Overview
The BH1750 is a digital light intensity sensor (lux meter) manufactured by ROHM Semiconductor. This sensor uses the I2C communication interface, allowing digitized light intensity data (in lux) to be transmitted directly to a microcontroller without the need for analog signal processing.

![bh1750-cam-bien-anh-sang-2](https://github.com/user-attachments/assets/16278d51-56c3-4bba-b05f-62de6c39addd)

The BH1750 has six different measurement modes which are divided in two groups; continuous and one-time measurements. In continuous mode the sensor continuously measures lightness value. In one-time mode, the sensor makes only one measurement and then goes into Power Down mode.

Each mode has three different precisions:

Low Resolution Mode - (4 lx precision, 16ms measurement time)
High Resolution Mode - (1 lx precision, 120ms measurement time)
High Resolution Mode 2 - (0.5 lx precision, 120ms measurement time)
# System Requirements
- Hardware

BH1750 sensor module
Embedded platform (Raspberry Pi, STM32MP1, etc.) with I2C support

- Software

Linux Kernel version 4.14 or later

Kernel configuration:
CONFIG_I2C
CONFIG_I2C_CHARDEV
CONFIG_MISCDEV

Development tools: gcc, make, i2c-tools
# Hardware Connections
- VCC -> 3.3 / 5V
- GND -> GND
- SDA -> SDA(GPIO 2)
- SCL -> SCL(GPIO 3)
- ADDR -> NC/GND or VCC (see below)

  The ADDR pin is used to set the sensor I2C address. By default (if ADDR voltage less than 0.7 * VCC) the sensor address will be 0x23. If it has voltage greater or equal to 0.7VCC voltage (e.g. you've connected it to VCC) the sensor address will be 0x5C.

Wiring up the GY-30 sensor board to an Raspberry is shown in the diagram below.
  
![download](https://github.com/user-attachments/assets/eafc8cf5-94fb-454d-a4bb-8983395b6595)

# Device Tree Overlay Setup
- Enable I2C (on Raspberry Pi)
- 
`sudo raspi-config`

Interface Options → I2C → Enable
- Device Tree Overlay (Optional but recommended)

Create a file: `/boot/overlays/bh1750-overlay.dts`

`/dts-v1/;`

`/plugin/;`

`/ { `

    `compatible = "brcm,bcm2835";`

    fragment@0 {
        target = <&i2c1>;
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;

            bh1750@23 {
                compatible = "rohm,BH1750";
                reg = <0x23>;   // Or 0x5C depending on ADDR pin
            };
        };
    };
`};`

Compile the overlay:

`dtc -@ -I dts -O dtb -o bh1750.dtbo bh1750-overlay.dts
sudo cp bh1750.dtbo /boot/overlays/`

Edit `/boot/config.txt:
dtoverlay=bh1750`

Reboot:

`sudo reboot`

# Build and Load the Kernel Module
- Build Driver

Ensure your source code is in BH1750_driver.c, and you have a Makefile

- Compile

`make`
- Load the module

`sudo insmod BH1750_driver.ko`

- Check device node
  
`ls /dev/BH1750`

You should see `/dev/BH1750`

If not:

`dmesg | tail`

→ check for logs like BH1750 device initialized successfully.

# Test with User Space Application

Compile and run test program

`gcc -o test_bh1750 test_bh1750.c`

`sudo ./test_bh1750`

# Advanced IOCTL Commands
You can call different modes by replacing `IOCTL_READ_One_H_Mode1` with:

| Macro                             | Mode | Description        |
| --------------------------------- | ---- | ------------------ |
| `IOCTL_READ_Continuously_H_Mode1` | 0x10 | 1 lx, continuous   |
| `IOCTL_READ_One_H_Mode1`          | 0x20 | 1 lx, one-shot     |
| `IOCTL_READ_Continuously_H_Mode2` | 0x11 | 0.5 lx, continuous |
| `IOCTL_READ_One_H_Mode2`          | 0x21 | 0.5 lx, one-shot   |
| `IOCTL_READ_Continuously_L_Mode`  | 0x13 | 4 lx, continuous   |
| `IOCTL_READ_One_L_Mode`           | 0x23 | 4 lx, one-shot     |

# Notes and Troubleshooting
Use i2cdetect -y 1 to verify the sensor is detected.

The address should match your overlay (either 0x23 or 0x5C).

You can only read data after enough delay (180ms typical).

Sensor auto powers off after one-shot mode.

# Unload and Clean Up
`sudo rmmod BH1750_driver`

`make clean`







