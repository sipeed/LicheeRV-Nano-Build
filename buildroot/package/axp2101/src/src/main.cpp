#include "main.h"
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <poll.h>
#include <iostream>
#include <string>
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
        perror("PMU: Failed to open the i2c bus");
    }
    return file;
}

static int set_i2c_slave(int file, int addr)
{
    if (ioctl(file, I2C_SLAVE, addr) < 0) {
        perror("PMU: Failed to acquire bus access and/or talk to slave");
        return -1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    bool log_enabled = false;

    printf("PMU: Init axp2101 pmu\n");

    i2c_dev = open_i2c_bus(I2C_BUS);
    if (i2c_dev < 0) {
        printf("PMU: Open i2c bus error\n");
        return -1;
    } else {
        printf("PMU: Open i2c bus success\n");
    }

    if (set_i2c_slave(i2c_dev, AXP2101_SLAVE_ADDRESS) < 0) {
        close(i2c_dev);
        printf("PMU: Set i2c slave error\n");
        return 1;
    } else {
        printf("PMU: Set i2c slave success\n");
    }

    if (!PMU) {
        if (ioctl(i2c_dev, I2C_SLAVE, AXP2101_SLAVE_ADDRESS) < 0)  {
            printf("PMU: Failed to access bus.\n");
            exit(1);
        }
        PMU = new XPowersAXP2101(AXP2101_SLAVE_ADDRESS, linux_i2c_read_callback, linux_i2c_write_callback);
        if (!PMU->init()) {
            printf("PMU: Failed to find AXP2101 power management\n");
            delete PMU;
            PMU = NULL;
        } else {
            printf("PMU: AXP2101 PMU init succeeded, using AXP2101 PMU\n");
        }
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--log" || arg == "-l") {
            log_enabled = true;
            break;
        }
    }

    if (log_enabled) {
        std::cout << "=======================================================================\n";

        if (PMU->isChannelAvailable(XPOWERS_DCDC1)) {
            std::cout << "DC1  : " << (PMU->isPowerChannelEnable(XPOWERS_DCDC1) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_DCDC1) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_DCDC2)) {
            std::cout << "DC2  : " << (PMU->isPowerChannelEnable(XPOWERS_DCDC2) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_DCDC2) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_DCDC3)) {
            std::cout << "DC3  : " << (PMU->isPowerChannelEnable(XPOWERS_DCDC3) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_DCDC3) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_DCDC4)) {
            std::cout << "DC4  : " << (PMU->isPowerChannelEnable(XPOWERS_DCDC4) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_DCDC4) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_DCDC5)) {
            std::cout << "DC5  : " << (PMU->isPowerChannelEnable(XPOWERS_DCDC5) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_DCDC5) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_ALDO1)) {
            std::cout << "ALDO1: " << (PMU->isPowerChannelEnable(XPOWERS_ALDO1) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_ALDO1) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_ALDO2)) {
            std::cout << "ALDO2: " << (PMU->isPowerChannelEnable(XPOWERS_ALDO2) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_ALDO2) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_ALDO3)) {
            std::cout << "ALDO3: " << (PMU->isPowerChannelEnable(XPOWERS_ALDO3) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_ALDO3) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_ALDO4)) {
            std::cout << "ALDO4: " << (PMU->isPowerChannelEnable(XPOWERS_ALDO4) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_ALDO4) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_BLDO1)) {
            std::cout << "BLDO1: " << (PMU->isPowerChannelEnable(XPOWERS_BLDO1) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_BLDO1) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_BLDO2)) {
            std::cout << "BLDO2: " << (PMU->isPowerChannelEnable(XPOWERS_BLDO2) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_BLDO2) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_DLDO1)) {
            std::cout << "DLDO1: " << (PMU->isPowerChannelEnable(XPOWERS_DLDO1) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_DLDO1) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_DLDO2)) {
            std::cout << "DLDO2: " << (PMU->isPowerChannelEnable(XPOWERS_DLDO2) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_DLDO2) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_CPULDO)) {
            std::cout << "CPUSLDO: " << (PMU->isPowerChannelEnable(XPOWERS_CPULDO) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_CPULDO) << " mV \n";
        }
        if (PMU->isChannelAvailable(XPOWERS_VBACKUP)) {
            std::cout << "VBACKUP: " << (PMU->isPowerChannelEnable(XPOWERS_VBACKUP) ? "+" : "-") 
                    << "   Voltage:" << PMU->getPowerChannelVoltage(XPOWERS_VBACKUP) << " mV \n";
        }

        std::cout << "=======================================================================\n";

        printPMU();

        if (PMU != nullptr) {
            delete PMU;
            PMU = NULL;
        }
        return 0;
    }

    PMU->setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_3V88);
    PMU->setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_2000MA);
    PMU->setSysPowerDownVoltage(2600);

    // DCDC1  -->  VDD3V3_SYS
    PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
    PMU->enablePowerOutput(XPOWERS_DCDC1);

    // DCDC2  -->  Disable
    PMU->disablePowerOutput(XPOWERS_DCDC2);

    // DCDC3  -->  VDD0V9_CPU
    PMU->setPowerChannelVoltage(XPOWERS_DCDC3, 1000);
    PMU->enablePowerOutput(XPOWERS_DCDC3);

    // DCDC4  -->  VDD1V35_DRAM
    PMU->setPowerChannelVoltage(XPOWERS_DCDC4, 1400);
    PMU->enablePowerOutput(XPOWERS_DCDC4);

    // DCDC5  -->  Disable
    PMU->disablePowerOutput(XPOWERS_DCDC5);

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

    PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
    PMU->clearIrqStatus();

    PMU->setLinearChargerVsysDpm(XPOWERS_AXP2101_VSYS_VOL_4V8);
    PMU->setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_200MA);
    PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_1000MA);
    PMU->setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
    PMU->disableChargerTerminationLimit();
    PMU->setChargingLedMode(XPOWERS_CHG_LED_OFF);
    PMU->enableCellbatteryCharge();
    
    // Set the time of pressing the button to turn off
    PMU->setPowerKeyPressOffTime(XPOWERS_POWEROFF_4S);
    // Set the button power-on press time
    PMU->setPowerKeyPressOnTime(XPOWERS_POWERON_128MS);

    PMU->disableTSPinMeasure();
    PMU->enableVbusVoltageMeasure();
    PMU->enableBattVoltageMeasure();
    PMU->enableSystemVoltageMeasure();
    PMU->enableBattDetection();

    PMU -> fuelGaugeControl(true, true);

    if (PMU != nullptr) {
        delete PMU;
        PMU = NULL;
    }

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

void printPMU() {
    std::cout << "---------------------------------------------------------------------------------------------------------\n";
    std::cout << "CHARG   DISC   STBY    VBUSIN    VGOOD    VBAT   VBUS   VSYS   Percentage    CHG_STATUS\n";
    std::cout << "(bool)  (bool) (bool)  (bool)    (bool)   (mV)   (mV)   (mV)      (%%)           (str)  \n";
    std::cout << "---------------------------------------------------------------------------------------------------------\n";

    std::cout << (PMU->isCharging() ? "YES" : "NO ") << "\t";
    std::cout << (PMU->isDischarge() ? "YES" : "NO ") << "\t";
    std::cout << (PMU->isStandby() ? "YES" : "NO ") << "\t";
    std::cout << (PMU->isVbusIn() ? "YES" : "NO ") << "\t";
    std::cout << (PMU->isVbusGood() ? "YES" : "NO ") << "\t";

    std::cout << PMU->getBattVoltage() << "\t";
    std::cout << PMU->getVbusVoltage() << "\t";
    std::cout << PMU->getSystemVoltage() << "\t";

    std::cout << PMU->getBatteryPercent() << "%\t";

    uint8_t charge_status = PMU->getChargerStatus();
    if (charge_status == XPOWERS_AXP2101_CHG_TRI_STATE) {
        std::cout << "tri_charge";
    } else if (charge_status == XPOWERS_AXP2101_CHG_PRE_STATE) {
        std::cout << "pre_charge";
    } else if (charge_status == XPOWERS_AXP2101_CHG_CC_STATE) {
        std::cout << "constant charge(CC)";
    } else if (charge_status == XPOWERS_AXP2101_CHG_CV_STATE) {
        std::cout << "constant voltage(CV)";
    } else if (charge_status == XPOWERS_AXP2101_CHG_DONE_STATE) {
        std::cout << "charge done";
    } else if (charge_status == XPOWERS_AXP2101_CHG_STOP_STATE) {
        std::cout << "not charging";
    }
    
    std::cout << "\n";
}
