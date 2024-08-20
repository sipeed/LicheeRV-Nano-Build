#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>
#include "XPowersLib.h"

#define I2C_BUS                 (4)
#define AXP2101_SLAVE_ADDRESS   (0x34)

int i2c_dev = -1;
XPowersLibInterface *PMU = NULL;

static int linux_i2c_read_callback(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
static int linux_i2c_write_callback(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len);
static void printPMU();

static int open_i2c_bus(int bus)
{
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", bus);
    int file = open(filename, O_RDWR);
    if (file < 0) {
        perror("Failed to open the i2c bus");
    }
    return file;
}

static int set_i2c_slave(int file, int addr)
{
    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    printf("Init axp2101 pmu\n");

    i2c_dev = open_i2c_bus(I2C_BUS);
    if (i2c_dev < 0) {
        printf("open i2c bus error\n");
        return -1;
    } else {
        printf("open i2c bus success\n");
    }

    if (set_i2c_slave(i2c_dev, AXP2101_SLAVE_ADDRESS) < 0) {
        close(i2c_dev);
        printf("set i2c slave error\n");
        return 1;
    } else {
        printf("set i2c slave success\n");
    }

    if (!PMU) {
        if (ioctl(i2c_dev, I2C_SLAVE, AXP2101_SLAVE_ADDRESS) < 0)  {
            printf("Failed to access bus.\n");
            exit(1);
        }
        PMU = new XPowersAXP2101(AXP2101_SLAVE_ADDRESS, linux_i2c_read_callback, linux_i2c_write_callback);
        if (!PMU->init()) {
            printf("Warning: Failed to find AXP2101 power management\n");
            delete PMU;
            PMU = NULL;
        } else {
            printf("AXP2101 PMU init succeeded, using AXP2101 PMU\n");
        }
    }

    // DCDC1  -->  VDD3V3_SYS
    PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
    PMU->enablePowerOutput(XPOWERS_DCDC1);

    // DCDC2  -->  Disable
    PMU->disablePowerOutput(XPOWERS_DCDC2);

    // DCDC3  -->  VDD0V9_CPU
    PMU->setPowerChannelVoltage(XPOWERS_DCDC3, 1000);
    PMU->enablePowerOutput(XPOWERS_DCDC3);

    // DCDC4  -->  VDD1V35_DRAM
    PMU->setPowerChannelVoltage(XPOWERS_DCDC4, 1340);
    PMU->enablePowerOutput(XPOWERS_DCDC4);

    // DCDC5  -->  DCDC5_FB
    PMU->setPowerChannelVoltage(XPOWERS_DCDC5, 1200);
    PMU->enablePowerOutput(XPOWERS_DCDC5);

    // ALDO1  -->  VDD1V8_SYS
    PMU->setPowerChannelVoltage(XPOWERS_ALDO1, 1800);
    PMU->enablePowerOutput(XPOWERS_ALDO1);

    // ALDO2 ~ ALDO4  -->  Disable
    PMU->disablePowerOutput(XPOWERS_ALDO2);
    PMU->disablePowerOutput(XPOWERS_ALDO3);
    PMU->disablePowerOutput(XPOWERS_ALDO4);

    // BLDO1 ~ BLDO2  -->  Disable
    PMU->disablePowerOutput(XPOWERS_BLDO1);
    PMU->disablePowerOutput(XPOWERS_BLDO2);

    // DLDO1 ~ DLDO2  -->  Disable
    PMU->disablePowerOutput(XPOWERS_DLDO1);
    PMU->disablePowerOutput(XPOWERS_DLDO2);

    // CPUSLDO  -->  Disable
    PMU->disablePowerOutput(XPOWERS_CPULDO);

    // VBACKUP
    PMU->setPowerChannelVoltage(XPOWERS_VBACKUP, 3100);
    PMU->enablePowerOutput(XPOWERS_VBACKUP);

    printf("=======He================================================================\n");
    if (PMU->isChannelAvailable(XPOWERS_DCDC1)) {
        printf("DC1  : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC1)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC1));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC2)) {
        printf("DC2  : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC2)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC2));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC3)) {
        printf("DC3  : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC3)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC3));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC4)) {
        printf("DC4  : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC4)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC4));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC5)) {
        printf("DC5  : %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DCDC5)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DCDC5));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO1)) {
        printf("ALDO1: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO1)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO1));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO2)) {
        printf("ALDO2: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO2)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO2));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO3)) {
        printf("ALDO3: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO3)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO3));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO4)) {
        printf("ALDO4: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_ALDO4)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_ALDO4));
    }
    if (PMU->isChannelAvailable(XPOWERS_BLDO1)) {
        printf("BLDO1: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_BLDO1)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_BLDO1));
    }
    if (PMU->isChannelAvailable(XPOWERS_BLDO2)) {
        printf("BLDO2: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_BLDO2)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_BLDO2));
    }
    if (PMU->isChannelAvailable(XPOWERS_BLDO1)) {
        printf("DLDO1: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DLDO1)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DLDO1));
    }
    if (PMU->isChannelAvailable(XPOWERS_BLDO2)) {
        printf("DLDO2: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_DLDO2)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_DLDO2));
    }
    if (PMU->isChannelAvailable(XPOWERS_CPULDO)) {
        printf("CPUSLDO: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_CPULDO)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_CPULDO));
    }
    if (PMU->isChannelAvailable(XPOWERS_VBACKUP)) {
        printf("VBACKUP: %s   Voltage:%u mV \n",  PMU->isPowerChannelEnable(XPOWERS_VBACKUP)  ? "+" : "-",  PMU->getPowerChannelVoltage(XPOWERS_VBACKUP));
    }
    printf("=======================================================================\n");

    PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_1000MA);
    PMU->setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
    PMU->setSysPowerDownVoltage(2600);
    PMU->setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);

    // Set the time of pressing the button to turn off
    PMU->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    // Set the button power-on press time
    PMU->setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);
    PMU->disableTSPinMeasure();
    PMU->enableBattDetection();
    PMU->enableVbusVoltageMeasure();
    PMU->enableBattVoltageMeasure();
    PMU->enableSystemVoltageMeasure();

    // printPMU();

    return 0;
}

static int linux_i2c_read_callback(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len)
{
    if (write(i2c_dev, &regAddr, 1) != 1) {
        perror("Failed to write to the i2c device");
        close(i2c_dev);
        return -1;
    }

    if (read(i2c_dev, data, len) != len) {
        perror("Failed to read from the i2c device");
        close(i2c_dev);
        return -1;
    }

    return 0;
}

static int linux_i2c_write_callback(uint8_t devAddr, uint8_t regAddr, uint8_t *data, uint8_t len)
{
    uint8_t buffer[len + 1];
    buffer[0] = regAddr;
    for (int i = 0; i < len; i++) {
        buffer[i + 1] = data[i];
    }

    if (write(i2c_dev, buffer, len + 1) != len + 1) {
        perror("Failed to write to the i2c device");
        close(i2c_dev);
        return -1;
    }

    return 0;
}

void printPMU()
{
    printf("isCharging:%s\n", PMU->isCharging() ? "YES" : "NO");
    printf("isDischarge:%s\n", PMU->isDischarge() ? "YES" : "NO");
    printf("isVbusIn:%s\n", PMU->isVbusIn() ? "YES" : "NO");
    printf("getBattVoltage:%u mV\n", PMU->getBattVoltage());
    printf("getVbusVoltage:%u mV\n", PMU->getVbusVoltage());
    printf("getSystemVoltage:%u mV\n", PMU->getSystemVoltage());

    // The battery percentage may be inaccurate at first use, the PMU will automatically
    // learn the battery curve and will automatically calibrate the battery percentage
    // after a charge and discharge cycle
    if (PMU->isBatteryConnect()) {
        printf("getBatteryPercent:%d%%", PMU->getBatteryPercent());
    }

    printf("\n");
}
