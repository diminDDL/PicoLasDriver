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
        out |= (*data >> bits_read) & 1; // item a) work from the least significant bits
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
    // item b) "push out" the last 16 bits
    int i;
    for (i = 0; i < 16; ++i) {
        bit_flag = out >> 15;
        out <<= 1;
        if(bit_flag)
            out ^= CRC16;
    }
    // item c) reverse the bits
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
bool Memory::readPage(uint8_t page){
    byte* b = EEPROM.readAddress(base_data_addr + page * page_size);
    Configuration c;
    memcpy(&c, b, sizeof(Configuration));
    // print the values
    Serial.println();
    Serial.print("current: ");
    Serial.println(c.current);
    Serial.print("maxcurrent: ");
    Serial.println(c.maxcurr);
    Serial.print("pulseduration: ");
    Serial.println(c.pulsedur);
    Serial.print("pulsefrequency: ");
    Serial.println(c.pulsefreq);
    Serial.print("analog: ");
    Serial.println(c.analog);
    // read the CRC
    Serial.print("CRC: ");
    uint16_t crcRead = (EEPROM.read((base_crc_addr + 1) + page * page_size) << 8) | EEPROM.read(base_crc_addr + page * page_size);
    Serial.println(crcRead, HEX);
    // calculate the CRC
    uint16_t crcCalc = gen_crc16(b, sizeof(Configuration));
    Serial.print("CRC calc: ");
    Serial.println(crcCalc, HEX);
    // check if the CRC is correct
    if (crcRead == crcCalc){
        Serial.println("CRC correct");
        // set the values
        config.globalpulse = c.globalpulse;
        config.current = c.current;
        config.maxcurr = c.maxcurr;
        config.pulsedur = c.pulsedur;
        config.pulsefreq = c.pulsefreq;
        config.analog = c.analog;
        return true;
    } else {
        Serial.println("CRC incorrect");
        return false;
    }
}

/*
* Read a page from the EEPROM, store the result regardless of the CRC, return true if CRC is correct
* @param page the page to read
* @param c the config struct to store the result in
*/
bool Memory::readPage(uint8_t page, Configuration &c){
    byte* b = EEPROM.readAddress(base_data_addr + page * page_size);
    memcpy(&c, b, sizeof(Configuration));
    // read the CRC
    uint16_t crcRead = (EEPROM.read((base_crc_addr + 1) + page * page_size) << 8) | EEPROM.read(base_crc_addr + page * page_size);
    // calculate the CRC
    uint16_t crcCalc = gen_crc16(b, sizeof(Configuration));
    // check if the CRC is correct
    if (crcRead == crcCalc){
        return true;
    } else {
        return false;
    }
}

bool Memory::writePage(uint8_t page, Configuration &c){
    // write the config
    byte* b = (byte*) &c;
    bool writeConf = EEPROM.write(base_data_addr + page * page_size, b, sizeof(Configuration));
    // calculate the CRC
    uint16_t crc = gen_crc16(b, sizeof(Configuration));
    // write the CRC
    bool writeCRC1 = EEPROM.write(base_crc_addr + page * page_size, crc & 0xFF);
    bool writeCRC2 = EEPROM.write((base_crc_addr + 1) + page * page_size, (crc >> 8) & 0xFF);
    return writeConf && writeCRC1 && writeCRC2;
}

/*
* Write a page to the EEPROM, this writes the config struct and the CRC
* @param page the page to write to
*/
bool Memory::writePage(uint8_t page){
    // first we read a page and see if the config changed
    Configuration c;
    bool correct = readPage(page, c);
    Serial.print("Read page");
    if (correct){
        // check if the config changed
        if (config.globalpulse == c.globalpulse && config.current == c.current && config.maxcurr == c.maxcurr && config.pulsedur == c.pulsedur && config.pulsefreq == c.pulsefreq && config.analog == c.analog){
            // the config didn't change, so we don't need to write it
            Serial.println(" - no change");
            return true;
        }else{
            Serial.println(" - changed");
            Serial.println("Writing page");
            // the config changed, so we need to write it
            return writePage(page, config);
        }
    }else{
        // the page was not correct, so we need to write it
        Serial.println(" - incorrect");
        Serial.println("Writing page");
        return writePage(page, config);
    }

}