#ifndef MEMORY_H
#define MEMORY_H
#include <Arduino.h>
#include <DueFlashStorage.h>

class Memory{
    private:
        DueFlashStorage EEPROM;                             // EEPROM object
        const uint8_t base_data_addr = 4;                   // address of the data in the EEPROM
        const uint8_t base_crc_addr = 0;                    // address of the crc in the EEPROM
        const uint8_t number_of_pages = 8;                  // number of pages to use for the EEPROM wear leveling
        const uint8_t page_size = IFLASH0_PAGE_SIZE;        // size of a page in the EEPROM
        const uint16_t CRC16 = 0x8005;                      // CRC16 polynomial
        // the settings we want to store in the EEPROM:
        // globalPulseCount
        // setCurrent
        // maxCurrent
        // setPulseDuration
        // setPulseFrequency
        // analogMode
        // the rest are reset on power cycle
        struct __attribute__((__packed__)) Configuration {
            uint64_t globalpulse;
            float current;
            float maxcurr;
            uint32_t pulsedur;
            uint32_t pulsefreq;
            bool analog;
        };
    public:
        Configuration config;                               // the struct we want to store in the EEPROM    
        Memory();                                           // constructor
        bool writePage(uint8_t page);                       // write the config to a page to the EEPROM includes CRC and struct
        bool writePage(uint8_t page, Configuration &c);     // write a config to a page to the EEPROM includes CRC and struct
        bool writeLeveled();                                // write the config to the EEPROM with wear leveling
        bool readPage(uint8_t page);                        // read a page from the EEPROM to the the config, checking CRC
        bool readPage(uint8_t page, Configuration &c);      // read a page from the EEPROM, store the result regardless of the CRC, return true if CRC is correct
        bool readLeveled();                                 // read the EEPROM with wear leveling
        uint16_t gen_crc16(const uint8_t *data, uint16_t size); // generate a CRC16 checksum
};

#endif