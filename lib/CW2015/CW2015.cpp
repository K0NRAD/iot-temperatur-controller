#include <Arduino.h>
#include <CW2015.h>
#include <Wire.h>
#include <esp32-hal-log.h>

#define NO_ERR 0

uint8_t ConfigInfoController[SIZE_BATINFO] = {
        0X15, 0X7E, 0X88, 0X68, 0X37, 0X1B, 0X2E, 0X4F,
        0X61, 0X4D, 0X3A, 0X43, 0X53, 0X5B, 0X5D, 0X63,
        0X5B, 0X4E, 0X44, 0X3F, 0X43, 0X48, 0X4F, 0X5A,
        0X56, 0X4C, 0X0A, 0X3E, 0X16, 0X2C, 0X2B, 0X34,
        0X36, 0X37, 0X3B, 0X3B, 0X3E, 0X19, 0X6C, 0X25,
        0X09, 0X0F, 0X6A, 0X8A, 0X90, 0X90, 0X90, 0X49,
        0X70, 0X88, 0X92, 0X94, 0X80, 0X34, 0X88, 0XCB,
        0X2F, 0X00, 0X64, 0XA5, 0XB5, 0X1D, 0XB8, 0X09};

uint8_t ConfigInfoPowerbank[SIZE_BATINFO] = {
        0X15, 0X7E, 0X86, 0X69, 0X3F, 0X1B, 0X2F, 0X50,
        0X61, 0X4F, 0X40, 0X49, 0X54, 0X57, 0X5E, 0X64,
        0X5A, 0X4D, 0X44, 0X43, 0X46, 0X4D, 0X56, 0X5C,
        0X4B, 0X9B, 0X06, 0X66, 0X11, 0X22, 0X30, 0X32,
        0X3B, 0X3C, 0X3E, 0X41, 0X40, 0X1B, 0X6F, 0X26,
        0X09, 0X10, 0X68, 0X89, 0X8F, 0X90, 0X90, 0X4A,
        0X7A, 0X89, 0X91, 0X91, 0X80, 0X20, 0X5D, 0XCB,
        0X2F, 0X00, 0X64, 0XA5, 0XB5, 0X1D, 0XB8, 0X11};

uint8_t CW2015::readByte(uint8_t address, uint8_t *reg_val) {
    Wire.beginTransmission(CW2015_ADDR);
    Wire.write(address);
    if (Wire.endTransmission()) {
        log_e("CW2015::readByte failed to prepare read from address %d", address);
        return 1;
    }

    Wire.requestFrom(CW2015_ADDR, 1);
    if (Wire.available()) {
        *reg_val = Wire.read();
        log_v("CW2015::readByte : read value %d from address %d", *reg_val, address);
        return NO_ERR;
    }
    log_e("CW2015::readByte failed to read byte from address: %d", address);
    return 1;
}

uint8_t CW2015::writeByte(uint8_t address, uint8_t reg_val) {
    Wire.beginTransmission(CW2015_ADDR);
    Wire.write(address);
    Wire.write(reg_val);
    if (Wire.endTransmission()) {
        log_e("CW2015::writeByte failed to write Byte %d", reg_val);
        return 1;
    }
    return NO_ERR;
}

float CW2015::batteryVoltage() {
    float voltage;
    uint8_t battery_lsb = 0;
    uint8_t battery_msb = 0;

    setMode(MODE_NORMAL);

    if (readByte(REG_VCELL, &battery_msb)) {
        return 0.0;
    }
    voltage = battery_msb << 8;

    if (readByte(REG_VCELL, &battery_lsb)) {
        return 0.0;
    }

    voltage += battery_lsb;

    // A 14 bit sigma-delta A/D converter is used and the voltage resolution is 305uV
    voltage = voltage * 305 / 1000000;

    return voltage;
}

float CW2015::batteryPercentage() {
    float percentage;
    uint8_t percent_lsb = 0;
    uint8_t percent_msb = 0;

    if (readByte(REG_SOC, &percent_msb)) {
        return 0.0;
    }
    percentage = (float) percent_msb;

    if (percentage < 100.00) {
        if (readByte(REG_SOC + 1, &percent_lsb)) {
            return 0.0;
        }
        percentage += (float(percent_lsb) / 256);
    }

    return percentage;
}

uint8_t CW2015::version() {
    uint8_t version = 0;

    if (readByte(REG_VERSION, &version)) {
        return 0;
    }

    return version;
}

uint8_t CW2015::mode() {
    uint8_t mode = 0;

    if (readByte(REG_MODE, &mode)) {
        return 0;
    }
    return mode;
}

uint8_t CW2015::setMode(uint8_t mode) {
    if (writeByte(REG_MODE, mode)) {
        log_e("Failed to set mode %d", mode);
        return 1;
    }
    return 0;
}

void CW2015::sleep() {
    setMode(MODE_SLEEP);
}

void CW2015::quickStart() {
    setMode(MODE_QUICK_START);
}

uint8_t CW2015::init_config_info(uint8_t config_info) {
    uint8_t i;
    uint8_t reg_val;
    uint8_t *cw_bat;

    if (config_info == CONFIG_INFO_CONTROLLER) {
        cw_bat = ConfigInfoController;
    } else {
        cw_bat = ConfigInfoPowerbank;
    }

    if (setMode(MODE_NORMAL)) {
        log_e("Init config info. Wakeup failed");
        return 1;
    }

    if (readByte(REG_CONFIG, &reg_val)) {
        log_e("REG_CONFIG failed to read");
        return 1;
    }
    //  alert threshold setting register
    if ((reg_val & 0xf8) != ATHD) {
        reg_val &= 0x07; // clear ATHD 0000 0111
        reg_val |= ATHD; // set ATHD
        if (writeByte(REG_CONFIG, reg_val)) {
            log_e("Set ATHD (alert threshold setting register) failed");
            return 1;
        }
    }

    if (readByte(REG_CONFIG, &reg_val)) {
        log_e("REG_CONFIG failed to read");
        return 1;
    }

    if (!(reg_val & CONFIG_UPDATE_FLG)) {
        log_i("Update flag for new battery info need set ");
        if (update_config_info(cw_bat)) {
            log_e("Update config failed");
            return 1;
        }
    } else {
        for (i = 0; i < SIZE_BATINFO; i++) {
            log_v("Validate battery info. Value: %x", cw_bat[i]);
            if (readByte((REG_BATINFO + i), &reg_val)) {
                log_e("REG_BATINFO + %d failed to read", i);
                return 1;
            }

            if (cw_bat[i] != reg_val)
                break;
        }

        if (i != SIZE_BATINFO) {
            log_w("Config didn't match, need update config");
            if (update_config_info(cw_bat)) {
                log_e("Update config failed");
                return 1;
            }
        }
    }

    delay10ms(10);

    for (i = 0; i < 30; i++) {
        // SOC State-of-Charge
        if (readByte(REG_SOC, &reg_val)) {
            log_e("REG_SOC failed to read");
            return 1;
        }
        log_v("State of charge: %d", reg_val);
        if (reg_val <= 0x64) {
            break;
        }
        delay10ms(100);
    }

    if (i >= 30) {
        setMode(MODE_SLEEP);
        log_w("cw2015 input unvalid power error, cw2015 join sleep mode");
    }

    log_i("cw2015 init success!");
    return 0;
}

uint8_t CW2015::update_config_info(uint8_t *cw_bat) {
    uint8_t reg_val;
    uint8_t reset_val;

    // make sure no in sleep mode
    if (readByte(REG_MODE, &reg_val)) {
        log_e("REG_MODE failed to read");
        return 1;
    }

    reset_val = reg_val;

    if ((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP) {
        log_e("CW2015 in sleep mode, update not possible");
        return 1;
    }

    log_v("Update battery info");
    for (int i = 0; i < SIZE_BATINFO; i++) {
        log_v("Update battery info. Value: %x", cw_bat[i]);
        if (writeByte(REG_BATINFO + i, cw_bat[i])) {
            log_e("Update battery info. REG_BATINFO + %d failed to write %d", i, cw_bat[i]);
            return 1;
        }
    }

    log_v("Validate updated battery info");
    for (int i = 0; i < SIZE_BATINFO; i++) {
        if (readByte(REG_BATINFO + i, &reg_val)) {
            log_e("Validate updated battery info. REG_BATINFO + %d failed to read", i);
            return 1;
        }
        if (cw_bat[i] != reg_val) {
            log_e("Validate updated battery info. Config didn't match");
            return 1;
        }
    }

    reg_val = 0x00;
    reg_val |= CONFIG_UPDATE_FLG; // set UPDATE_FLAG
    reg_val &= 0x07;              // clear ATHD
    reg_val |= ATHD;              // set ATHD
    if (writeByte(REG_CONFIG, reg_val)) {
        log_e("REG_CONFIG failed to write");
        return 1;
    }

    log_v("Check 2015for ATHD a update flag is set");
    if (readByte(REG_CONFIG, &reg_val)) {
        log_e("Set restart flag. REG_CONFIG failed to read");
        return 1;
    }
    if (!(reg_val & CONFIG_UPDATE_FLG)) {
        log_w("Update flag for new battery info have not set..");
    }
    if ((reg_val & ATHD) != ATHD) {
        log_w("The new ATHD have not set..");
    }

    reg_val = 0x00;
    reset_val &= ~(MODE_RESTART);
    reg_val = reset_val | MODE_RESTART;
    if (writeByte(REG_MODE, reg_val)) {
        log_e("Set reset mode. REG_MODE failed to write");
        return 1;
    }

    delay10ms(1);

    log_i("cw2015 update config success!");
    return 0;
}

void CW2015::delay10ms(unsigned int c) {
    uint8_t a, b;
    for (; c > 0; c--) {
        for (b = 95; b > 0; b--) {
            for (a = 209; a > 0; a--);
        }
    }
}