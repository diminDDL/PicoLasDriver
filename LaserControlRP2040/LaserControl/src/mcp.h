#include <Arduino.h>

class MCP{
    private:
        arduino::MbedI2C* _MCP_I2C;
        uint8_t address = 0b01100000;   // TODO make this correct

    public:
        MCP(arduino::MbedI2C* MCP_I2C, uint8_t address = 0b01100000){
            this->_MCP_I2C = MCP_I2C;
            this->address = address;
        }

        bool init(){
            this->_MCP_I2C->begin();
            this->_MCP_I2C->setClock(100000);   // TODO increase speed
            // reset and wait for ACK
            this->_MCP_I2C->beginTransmission(this->address);
            return this->_MCP_I2C->endTransmission() == 0;
        }

        bool write(uint8_t reg, uint8_t data){
            // TODO check if this is correct
            this->_MCP_I2C->beginTransmission(this->address);
            this->_MCP_I2C->write(reg);
            this->_MCP_I2C->write(data);
            return this->_MCP_I2C->endTransmission() == 0;
        }

        bool writeValue(uint16_t value){
            // TODO check if this is correct
            this->_MCP_I2C->beginTransmission(this->address);
            this->_MCP_I2C->write(value >> 8);
            this->_MCP_I2C->write(value & 0xFF);
            return this->_MCP_I2C->endTransmission() == 0;
        }

        // note: https://github.com/RobTillaart/MCP4725/blob/master/MCP4725.cpp
};