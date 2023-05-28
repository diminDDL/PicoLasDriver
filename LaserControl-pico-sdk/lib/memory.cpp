#include "memory.hpp"
#include "fram.hpp"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* 
* Constructor
* @param FRAM_SPI pointer to the SPI object for the FRAM
* @param FRAM_CS the chip select pin for the FRAM
*/

Memory::Memory(spi_inst_t* FRAM_SPI, unsigned char FRAM_CS){
    this->_FRAM = new FRAM(FRAM_SPI, FRAM_CS);
}

/*
* Initialize the memory (needs to be called at runtime)
*/
void Memory::init(){
    this->_FRAM->init();
}

/*
* Generate a CRC16 checksum
* @param data pointer to the data to be checksummed
* @param size size of the data
* @return the CRC16 checksum
*/
uint16_t Memory::gen_crc16(const uint8_t *data, uint16_t size)
{
    uint16_t out = 0;
    int bits_read = 0, bit_flag;
    /* Sanity check: */
    if(data == NULL)
        return 0;
    while(size > 0)
    {
        bit_flag = out >> 15;
        /* Get next bit: */
        out <<= 1;
        out |= (*data >> bits_read) & 1; // work from the least significant bits
        /* Increment bit counter: */
        bits_read++;
        if(bits_read > 7)
        {
            bits_read = 0;
            data++;
            size--;
        }
        /* Cycle check: */
        if(bit_flag)
            out ^= CRC16;
    }
    // "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }
    // reverse the bits
    uint16_t crc = 0;
    i = 0x8000;
    int j = 0x0001;
    for (; i != 0; i >>=1, j <<= 1) {
        if (i & out) crc |= j;
    }
    return crc;
}

/*
* Read an EEPROM page, checking the CRC and storing the read data into the the config struct
* @param page the page to read
* @return true if the page was read successfully
*/
bool Memory::readPage(uint32_t page){
    uint8_t b[sizeof(Configuration)];
    this->_FRAM->read((uint16_t)(base_data_addr + page * page_size), b, sizeof(Configuration));
    Configuration c;
    memcpy(&c, b, sizeof(Configuration));
    // read the CRC
    uint16_t crcRead = (this->_FRAM->read((base_crc_addr + 1) + page * page_size) << 8) | this->_FRAM->read(base_crc_addr + page * page_size);
    // calculate the CRC
    uint16_t crcCalc = gen_crc16(b, sizeof(Configuration));
    // check if the CRC is correct
    if (crcRead == crcCalc){
        // set the values
        config.globalpulse = c.globalpulse;
        config.current = c.current;
        config.maxcurr = c.maxcurr;
        config.pulsedur = c.pulsedur;
        config.pulsefreq = c.pulsefreq;
        config.analog = c.analog;
        return true;
    } else {
        return false;
    }
}

/*
* Read a page from the EEPROM, store the result regardless of the CRC, return true if CRC is correct
* @param page the page to read
* @param c the config struct to store the result in
* @return true if the page was read with a correct CRC
*/
bool Memory::readPage(uint32_t page, Configuration &c){
    uint8_t b[sizeof(Configuration)];
    this->_FRAM->read((uint16_t)(base_data_addr + page * page_size), b, sizeof(Configuration));
    memcpy(&c, b, sizeof(Configuration));
    // read the CRC
    uint16_t crcRead = (this->_FRAM->read((base_crc_addr + 1) + page * page_size) << 8) | this->_FRAM->read(base_crc_addr + page * page_size);
    // calculate the CRC
    uint16_t crcCalc = gen_crc16(b, sizeof(Configuration));
    // check if the CRC is correct
    if (crcRead == crcCalc){
        return true;
    } else {
        return false;
    }
}

/*
* Write a configuration to a page in the EEPROM, this writes the config struct and the CRC
* @param page the page address to write to
* @param c the config struct to store in the EEPROM
* @return true if the page was written successfully
*/
bool Memory::writePage(uint32_t page, Configuration &c){
    uint8_t* b = (uint8_t*) &c;
    this->_FRAM->write(base_data_addr + page * page_size, b, sizeof(Configuration));
    uint16_t crc = gen_crc16(b, sizeof(Configuration));
    // write the CRC
    this->_FRAM->write(base_crc_addr + page * page_size, crc & 0xFF);
    this->_FRAM->write((base_crc_addr + 1) + page * page_size, (crc >> 8) & 0xFF);
    // now we try reading the page and check the CRC
    bool read = readPage(page, c);
    return read;
}

/*
* Write a page to the EEPROM, this writes the config struct and the CRC
* @param page the page to write to
* @return true if the page was written successfully
*/
bool Memory::writePage(uint32_t page){
    // first we read a page and see if the config changed
    Configuration c;
    bool correct = readPage(page, c);
    if (correct){
        // check if the config changed
        if (config.globalpulse == c.globalpulse && config.current == c.current && config.maxcurr == c.maxcurr && config.pulsedur == c.pulsedur && config.pulsefreq == c.pulsefreq && config.analog == c.analog){
            // the config didn't change, so we don't need to write it
            return true;
        }else{
            // the config changed, so we need to write it
            return writePage(page, config);
        }
    }else{
        // the page was not correct, so we need to write it
        return writePage(page, config);
    }

}

/*
* Reads the most recent config from the EEPROM
* @param storeStruct if true, the read config is stored in the config struct, if false the global write variables are incremented
* @return true if the config was read successfully
*/
bool Memory::readLeveled(bool storeStruct){
    // Memory architecture:
    // we have pages, defined by page_size
    // when we write to flash we write one page at a time
    // we have a counter that keeps track of the written pages
    // higher number - more recent data
    // if the counter is higher than number_of_pages, we start overwriting the oldest pages
    // if it's 2*number_of_pages, we start overwriting the oldest pages again starting from 0 page (to avoid overflow)
    // we also have a CRC for each page
    // when we read a page, we check the CRC and if it's correct, we read the data
    // based on the above we set the page to write to
    // sotreStruct is used to store the read config in the config struct, if it's false, we don't store it and increment the global variables used for writing

    Configuration c[number_of_pages] = {0};     // stores a list of all the configurations (not very efficient, but it's fine)
    bool correct[number_of_pages] = {0};        // a list of the configurations with a correct CRC
    uint32_t index[number_of_pages] = {0};      // a list of the indexes of the configurations
    bool broke = false;                         // a flag to that indicates if we broke out of the reading loop
    bool overflow_writer = false;               
    bool overflow_reader = false;
    uint32_t max_valid_page = 0;                // a valid page with the highest index
    uint32_t min_valid_page = 99999;            // a valid page with the lowest index
    for (uint8_t i = 0; i < number_of_pages; ++i){
        correct[i] = readPage(i, c[i]);
        if(correct[i]){
            if(i > max_valid_page){
                max_valid_page = i;
            }
            if(i < min_valid_page){
                min_valid_page = i;
            }
        }
        index[i] = this->_FRAM->read((base_page_header) + i * page_size);
        if(!storeStruct){
            if(!correct[i]){
                // if we found a page with an incorrect CRC, we can just write here
                // if the page is not in the list of known bad pages, we try to write here
                if (!bad_pages[i]){
                    current_page = i;
                    if(i > 0){
                        current_index = index[i-1]+1;
                        if(current_index >= number_of_pages*2){
                            current_index = current_page;
                        }
                    }else{
                        current_index = 0;
                    }
                    broke = true;
                    break;
                }
            }
        }
    }

    uint32_t max_index = 0;
    uint32_t max_index_page = 0;

    if(!broke && !storeStruct){
        // to figure out the index we need to find if we are in overflow mode
        // overflow mode - number_of_pages < current_index < number_of_pages*2
        // normal mode - 0 < current_index < number_of_pages
        // to find if we are in overflow mode we check if the last page has a lower index than the first page
        // if it does, we are in overflow mode
        overflow_writer = index[max_valid_page] < index[min_valid_page];

        for (int i = 0; i < number_of_pages; ++i){
            if(((overflow_writer && index[i] > max_index) || (!overflow_writer && index[i] > max_index && index[i] < number_of_pages)) && !bad_pages[i]){
                max_index = index[i];
                max_index_page = i;
            }
        }

        uint32_t max_index_page_write = max_index_page+1;
        uint8_t count = number_of_pages;
        
        while(bad_pages[max_index_page_write] && count > 0){
            max_index_page_write++;
            // roll over
            if(max_index_page_write >= number_of_pages){
                max_index_page_write = 0;
                // if we weren't in overflow mode and had to roll over, we are now in overflow mode
                if(!overflow_writer)
                    overflow_writer = true;
                else
                    overflow_writer = false;
            }
            count--;
        }

        if(max_index_page_write >= number_of_pages){
            max_index_page_write = max_index_page_write - number_of_pages;
        }

        if(index[min_valid_page] == number_of_pages && index[max_valid_page] == number_of_pages*2-1){
            max_index_page_write = min_valid_page;
        }

        uint32_t max_index_write = 0;
        if(overflow_writer){
            max_index_write = max_index_page_write + number_of_pages;
        }else{
            max_index_write = max_index_page_write;
        }

        current_index = max_index_write;
        current_page = max_index_page_write;
        
    }
    
    if(storeStruct){
        overflow_reader = index[max_valid_page] < index[min_valid_page];
        if((index[min_valid_page] == min_valid_page + number_of_pages) && (index[max_valid_page] == (number_of_pages*2)-(number_of_pages-max_valid_page))){
            overflow_reader = true;
        }
        for (int i = 0; i < number_of_pages; ++i){
            if(((overflow_reader && index[i] > max_index) || (!overflow_reader && index[i] > max_index && index[i] < number_of_pages)) && !bad_pages[i]){
                max_index = index[i];
                max_index_page = i;
            }
        }
        // if the last page is incorrect, we need to find the last correct page
        int16_t count = number_of_pages;
        while(!correct[max_index_page] && count > 0){
            if(max_index_page == 0){
                max_index_page = number_of_pages;
            }
            max_index_page--;
            count--;
        }
        if(count == 0){
            return false;
        }
        config.globalpulse = c[max_index_page].globalpulse;
        config.current = c[max_index_page].current;
        config.maxcurr = c[max_index_page].maxcurr;
        config.pulsedur = c[max_index_page].pulsedur;
        config.pulsefreq = c[max_index_page].pulsefreq;
        config.analog = c[max_index_page].analog;
    }

    return correct[max_index_page];
}

/*
* Writes the current config to the EEPROM
* the user must populate the config struct with the correct values to be written
* @return true if the config was written successfully
*/
bool Memory::writeLeveled(){
    // try to write, if we can't read back the crc we add it to the list of bad pages and try the next page
    int16_t count = number_of_pages;
    bool correct = false;

    Configuration c;
    correct = readPage(current_page, c);
    if(correct){
        if (config.globalpulse == c.globalpulse && config.current == c.current && config.maxcurr == c.maxcurr && config.pulsedur == c.pulsedur && config.pulsefreq == c.pulsefreq && config.analog == c.analog){
            // printf("Config is the same, not writing\n");
            return true;
        }
    }

    do{
        // first we read the page but don't override the config struct
        readLeveled(false);
        correct = writePage(current_page);
        this->_FRAM->write((base_page_header) + current_page * page_size, current_index);
        if(!correct){
            bad_pages[current_page] = true;
            //printf("Bad page: %d\n", current_page);
        }
        //printf("Wrote to: %d\n", current_page);
        //printf("   Index: %d\n", current_index);
        count--;
    }while((bad_pages[current_page] || !correct) && count > 0);
    return correct;
}

/*
* Reads the current config from the EEPROM into the config struct
* @return true if the config was read successfully
*/
bool Memory::loadCurrent(){
    return readLeveled(true);
}
