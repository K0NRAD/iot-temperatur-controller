#ifndef CW2015_H
#define CW2015_H

#define CW2015_ADDR 0x62
#define REG_VERSION 0x0
#define REG_VCELL 0x2
#define REG_SOC 0x4
#define REG_RRT_ALERT 0x6
#define REG_CONFIG 0x8
#define REG_MODE 0xA
#define REG_BATINFO 0x10

/*
00 00 0000
|| || ||||_ PowerOnReset (POR) 
|| |_______ QuickStart  
||_________ Sleep
*/

#define MODE_SLEEP_MASK (0x3 << 6)
#define MODE_SLEEP (0x3 << 6)
#define MODE_NORMAL (0x0 << 6)
#define MODE_QUICK_START (0x3 << 4)
#define MODE_RESTART (0xF << 0)

#define CONFIG_UPDATE_FLG (0x1 << 1)
#define ATHD (0x0 << 3) //ATHD = 0%

#define SIZE_BATINFO 64

#define CONFIG_INFO_CONTROLLER 1
#define CONFIG_INFO_POWER_BANK 2

class CW2015 {

public:
    CW2015() = default;

    ~CW2015() = default;

    float batteryVoltage();

    float batteryPercentage();

    void quickStart();

    void sleep();

    void wakeup();

    uint8_t version();

    uint8_t init_config_info(uint8_t config_info);

private:
    uint8_t mode();

    uint8_t setMode(uint8_t mode);

    uint8_t readByte(uint8_t address, uint8_t *reg_val);

    uint8_t writeByte(uint8_t address, uint8_t reg_val);

    uint8_t update_config_info(uint8_t *cw_bat);

    void delay10ms(unsigned int c);

};

#endif