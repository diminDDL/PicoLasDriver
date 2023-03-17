#include <memory.h>


Memory::Memory(){
    // idk what to put in the constructor
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
* Read an EEPROM page, checking the CRC into the the config struct
* @param page the page to read
* @return true if the page was read successfully
*/
bool Memory::readPage(uint32_t page, bool dbg){
    byte* b = EEPROM.readAddress(base_data_addr + page * page_size);
    Configuration c;
    memcpy(&c, b, sizeof(Configuration));
    // read the CRC
    uint16_t crcRead = (EEPROM.read((base_crc_addr + 1) + page * page_size) << 8) | EEPROM.read(base_crc_addr + page * page_size);
    // calculate the CRC
    uint16_t crcCalc = gen_crc16(b, sizeof(Configuration));
    uint8_t idx = EEPROM.read((base_page_header) + page * page_size);
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
*/
bool Memory::readPage(uint32_t page, Configuration &c){
    // Serial.print("Struct Reading addr: ");
    // Serial.println(base_data_addr + page * page_size);
    byte* b = EEPROM.readAddress(base_data_addr + page * page_size);
    memcpy(&c, b, sizeof(Configuration));
    // read the CRC
    // Serial.print("reading CRC addr: ");
    // Serial.println(base_crc_addr + page * page_size);
    uint16_t crcRead = (EEPROM.read((base_crc_addr + 1) + page * page_size) << 8) | EEPROM.read(base_crc_addr + page * page_size);
    // calculate the CRC
    uint16_t crcCalc = gen_crc16(b, sizeof(Configuration));
    // check if the CRC is correct
    // Serial.print("CRC read: ");
    // Serial.println(crcRead, HEX);
    // Serial.print("CRC calc: ");
    // Serial.println(crcCalc, HEX);

    if (crcRead == crcCalc){
        return true;
    } else {
        return false;
    }
}

bool Memory::writePage(uint32_t page, Configuration &c){
    // write the config
    byte* b = (byte*) &c;
    EEPROM.write(base_data_addr + page * page_size, b, sizeof(Configuration));
    // calculate the CRC
    uint16_t crc = gen_crc16(b, sizeof(Configuration));


    // FOR TESTING REMOVE LATER
    // TODO REMOVE
    // if(page == 0 || page == 5 || page == 6 || page == 7 || page == 9)
    //     crc = 0x1234;


    // write the CRC
    EEPROM.write(base_crc_addr + page * page_size, crc & 0xFF);
    EEPROM.write((base_crc_addr + 1) + page * page_size, (crc >> 8) & 0xFF);
    // now we try reading the page and check the CRC
    bool read = readPage(page, c);
    return read;
}

/*
* Write a page to the EEPROM, this writes the config struct and the CRC
* @param page the page to write to
*/
bool Memory::writePage(uint32_t page){
    // first we read a page and see if the config changed
    Configuration c;
    bool correct = readPage(page, c);
    //Serial.print("Read page");
    if (correct){
        // check if the config changed
        if (config.globalpulse == c.globalpulse && config.current == c.current && config.maxcurr == c.maxcurr && config.pulsedur == c.pulsedur && config.pulsefreq == c.pulsefreq && config.analog == c.analog){
            // the config didn't change, so we don't need to write it
            // Serial.println(" - no change");
            return true;
        }else{
            // Serial.println(" - changed");
            // Serial.println("Writing page");
            // the config changed, so we need to write it
            return writePage(page, config);
        }
    }else{
        // the page was not correct, so we need to write it
        // Serial.println(" - incorrect");
        // Serial.println("Writing page");
        return writePage(page, config);
    }

}

/*
* Reads the most recent config from the EEPROM
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
    // when we read a page, we check the CRC
    // so in this function we want to just get the latest page with the valid CRC
    // Serial.println("===================Reading leveled===================");
    Configuration c[number_of_pages] = {0};   
    bool correct[number_of_pages] = {0};
    uint32_t index[number_of_pages] = {0};
    bool broke = false;
    bool overflow_writer = false;
    static bool last_mode_writer = false;
    bool overflow_reader = false;
    static bool last_mode_reader = false;
    uint32_t max_valid_page = 0; // a valid page with the highest index
    uint32_t min_valid_page = 99999; // a valid page with the lowest index
    for (int i = 0; i < number_of_pages; ++i){
        // Serial.print("--- Reading page ");
        // Serial.println(i);
        correct[i] = readPage(i, c[i]);
        if(correct[i]){
            if(i > max_valid_page){
                max_valid_page = i;
            }
            if(i < min_valid_page){
                min_valid_page = i;
            }
        }
        index[i] = EEPROM.read((base_page_header) + i * page_size);
        // Serial.print("Index: ");
        // Serial.println(index[i]);
        if(!storeStruct){
            if(correct[i]){
                // when we read a page with a correct CRC we check if we have overflowed and should start writing from the start again
                // Serial.println("Found a page with a correct CRC");
            }else{
                // Serial.print("Found a page with an incorrect CRC, page: ");
                // Serial.println(i);
                // if we found a page with an incorrect CRC, we can just write here,
                // if we already tried to write here (current_page = i) we shouldn't write again
                // this indicates a problem with the EEPROM on this page and after a failure we should continue regularly
                if (!bad_pages[i]){
                    // we found a page with an incorrect CRC, so we write here
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
                    // Serial.println("This page is not bad, writing");
                    break;
                    // TODO, bruh we need to maintain a list of bad pages
                }else{
                    // Serial.println("This page is bad, skipping");
                }
            }
        }

        // Serial.print("--- Page ");
        // Serial.print(i);
        // Serial.print(" index: ");
        // Serial.print(index[i]);
        // Serial.print(" correct: ");
        // Serial.println(correct[i]);
        // Serial.print("current: ");
        // Serial.println(c[i].current);
        // Serial.println("---------");
    }

    uint32_t max_index = 0;
    uint32_t max_index_page = 0;

    // Serial.print("Max Valid Page: ");
    // Serial.println(max_valid_page);
    // Serial.print("Min Valid Page: ");
    // Serial.println(min_valid_page);

    if(!broke && !storeStruct){
        // to figure out the index we need to find if we are in overflow mode
        // overflow mode - number_of_pages < current_index < number_of_pages*2
        // normal mode - 0 < current_index < number_of_pages
        // to find if we are in overflow mode we check if the last page has a lower index than the first page
        // if it does, we are in overflow mode
        // TODO, this is not reliable if one of these becomes bad
        // Serial.print("Index at max valid page: ");
        // Serial.println(index[max_valid_page]);
        // Serial.print("Index at min valid page: ");
        // Serial.println(index[min_valid_page]);
        overflow_writer = index[max_valid_page] < index[min_valid_page];
        // Serial.print("Overflow: ");
        // Serial.println(overflow_writer);
        // Serial.print("Last mode: ");
        // Serial.println(last_mode_writer);
        // Serial.println("///////////// finding index and page stuff here /////////////");
        for (int i = 0; i < number_of_pages; ++i){
            if(((overflow_writer && index[i] > max_index) || (!overflow_writer && index[i] > max_index && index[i] < number_of_pages)) && !bad_pages[i]){
                max_index = index[i];
                // Serial.print("Setting max_index to: ");
                // Serial.println(max_index);
                max_index_page = i;
                // Serial.print("Setting max_index_page to: ");
                // Serial.println(max_index_page);
            }
        }

        uint32_t max_index_write = 0;
        uint32_t max_index_page_write = max_index_page+1;
        uint8_t count = number_of_pages;
        // Serial.println("Checking for bad pages");
        while(bad_pages[max_index_page_write] && count > 0){
            max_index_page_write++;
            // roll over
            if(max_index_page_write >= number_of_pages){
                max_index_page_write = 0;
            }
            count--;
        }
        // Serial.print("1. max_index_page_write: ");
        // Serial.println(max_index_page_write);

        if(max_index_page_write >= number_of_pages){
            max_index_page_write = max_index_page_write - number_of_pages;
        }
        // Serial.print("2. max_index_page_write: ");
        // Serial.println(max_index_page_write);

        if(overflow_writer){
            max_index_write = max_index_page + number_of_pages;
        }else{
            max_index_write = max_index_page;
        }

        max_index_write = max_index_write+1;

        if(last_mode_writer && !overflow_writer){
            max_index_write = min_valid_page;
            max_index_page_write = min_valid_page;
        }
        last_mode_writer = overflow_writer;

        // Serial.print("3. max_index_page_write: ");
        // Serial.println(max_index_page_write);
        
        
        // Serial.print("Setting global");
        // Serial.print(" current_index: ");
        // Serial.print(max_index_write);
        // Serial.print("; current_page: ");
        // Serial.println(max_index_page_write);
        current_index = max_index_write;
        current_page = max_index_page_write;
        
        //Serial.println("///////////////////////////////////////");
    }
    
    if(storeStruct){
         // TODO, this is not reliable if one of these becomes bad
        overflow_reader = index[max_valid_page] < index[min_valid_page];

        // Serial.print("Overflow: ");
        // Serial.println(overflow_writer);
        // Serial.print("Last mode: ");
        // Serial.println(last_mode_writer);

        // Serial.println("\\\\\\\\\\\\\\ finding index and page stuff here \\\\\\\\\\\\\\");

        for (int i = 0; i < number_of_pages; ++i){
            if(((overflow_reader && (index[i] > max_index)) || (!overflow_reader && (index[i] > max_index) && (index[i] < number_of_pages))) && !bad_pages[i]){
                max_index = index[i];
                // Serial.print("Setting max_index to: ");
                // Serial.println(max_index);
                max_index_page = i;
                // Serial.print("Setting max_index_page to: ");
                // Serial.println(max_index_page);
            }
        }

        if(last_mode_reader && !overflow_reader){
            max_index_page = number_of_pages-1;
        }
        last_mode_reader = overflow_reader;

        // if the last page is incorrect, we need to find the last correct page
        int16_t count = number_of_pages;
        // this makes no sense wtf
        while(!correct[max_index_page] && count > 0){
            // Serial.print("Incorrect page: ");
            // Serial.println(max_index_page);
            if(max_index_page == 0){
                max_index_page = number_of_pages;
            }
            max_index_page--;
            // Serial.print("Trying: ");
            // Serial.println(max_index_page);
            count--;
        }
        if(count == 0){
            // Serial.println("No correct pages found");
            return false;
        }
        // Serial.print("Max index page: ");
        // Serial.println(max_index_page);
        config.globalpulse = c[max_index_page].globalpulse;
        config.current = c[max_index_page].current;
        config.maxcurr = c[max_index_page].maxcurr;
        config.pulsedur = c[max_index_page].pulsedur;
        config.pulsefreq = c[max_index_page].pulsefreq;
        config.analog = c[max_index_page].analog;
        // print the values
        // Serial.println("=====================================================");
        // Serial.println("Config values:");
        // Serial.print("Current: ");
        // Serial.println(config.current);
        // Serial.println("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
    }

    // Serial.println();
    // Serial.print("Current page: ");
    // Serial.println(current_page);
    // Serial.print("Current index: ");
    // Serial.println(current_index);
    // Serial.println("=====================================================");
    return correct[max_index_page];
}

/*
* Writes the current config to the EEPROM
* the user must run readLeveled() before this function
* and populate the config struct with the correct values after that
* @return true if the config was written successfully
*/
bool Memory::writeLeveled(){
    
    // try to write, if we can't read back the crc we add it to the list of bad pages and try the next page
    int16_t count = number_of_pages;
    bool correct = false;
    do{
        // first we read the page but don't override the config struct
        readLeveled(false);
        correct = writePage(current_page);
        correct &= EEPROM.write((base_page_header) + current_page * page_size, current_index);
        if(!correct){
            bad_pages[current_page] = true;
            Serial.print("Bad page: ");
            Serial.println(current_page);
        }
        Serial.print("Wrote to: ");
        Serial.println(current_page);
        Serial.print("Index: ");
        Serial.println(current_index);
        
        count--;
    }while((bad_pages[current_page] || !correct) && count > 0);
    return correct;
}
// TODO writeLeveled semi working, needs to handle index of pages after 1 run (isn't working now)

bool Memory::loadCurrent(){
    return readLeveled(true);
}
