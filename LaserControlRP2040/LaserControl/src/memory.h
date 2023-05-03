#ifndef MEMORY_H
#define MEMORY_H
#include <Arduino.h>

class Memory{
    private:
        //DueFlashStorage EEPROM;                             // EEPROM object
        const uint8_t base_data_addr = 4;                   // address of the data in the EEPROM
        const uint8_t base_crc_addr = 0;                    // address of the crc in the EEPROM (2 bytes)
        const uint8_t base_page_header = 2;                 // address of the page header in the EEPROM
        // TODO increase the number of pages
        static const uint8_t number_of_pages = 8;
        // static const uint8_t number_of_pages = 255;          // number of pages to use for the EEPROM wear leveling
        bool bad_pages[number_of_pages] = {0};              // array to store the bad pages
        uint8_t current_page = 0;                           // the current page we are writing to
        uint32_t current_index = 0;                         // the current index (ID of the blocks we are writing)
        const uint32_t page_size = 256;       // size of a page in the EEPROM
        const uint16_t CRC16 = 0x8005;                      // CRC16 polynomial

        struct __attribute__((__packed__)) Configuration {
            uint64_t globalpulse;
            float current;
            float maxcurr;
            uint32_t pulsedur;
            uint32_t pulsefreq;
            bool analog;
        };
        
        bool readLeveled(bool storeStruct = true);              // read the EEPROM with wear leveling, used internally can break when called incorrectly
    public:
        Configuration config;                                   // the struct we want to store in the EEPROM    
        Memory();                                               // constructor
        bool writePage(uint32_t page);                          // write the config to a page to the EEPROM includes CRC and struct
        bool writePage(uint32_t page, Configuration &c);        // write a config to a page to the EEPROM includes CRC and struct
        bool writeLeveled();                                    // write the config to the EEPROM with wear leveling
        bool readPage(uint32_t page);                           // read a page from the EEPROM to the the config, checking CRC
        bool readPage(uint32_t page, Configuration &c);         // read a page from the EEPROM, store the result regardless of the CRC, return true if CRC is correct
        bool loadCurrent();                                     // loads the latest config from the EEPROM
        uint16_t gen_crc16(const uint8_t *data, uint16_t size); // generate a CRC16 checksum
};

#endif