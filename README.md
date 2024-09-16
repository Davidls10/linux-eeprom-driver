# Linux Device Driver for at24c256 EEPROM
This project aims to enable communication with an EEPROM on a generic i2c bus
through a device file.

1. **Execute make in the directory with the at24c256.c and Makefile files**

2. **Insert the module into the kernel:** sudo insmod at24c256.ko

3. **Enable access to the device file:** sudo chmod 777 /dev/at24c256

4. **To read from:** less -f /dev/at24c256

5. **To write to:** echo "message here" | sudo tee /dev/at24c256 
