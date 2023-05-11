#include <Arduino.h>
#include <SPI.h>

class FRAM{
    private:
        uint8_t WREN  = 0b00000110;     // Set write enable latch
        uint8_t WRDI  = 0b00000100;     // Write disable
        uint8_t RDSR  = 0b00000101;     // Read status register
        uint8_t WRSR  = 0b00000001;     // Write status register
        uint8_t READ  = 0b00000011;     // Read memory data
        uint8_t WRITE = 0b00000010;     // Write memory data

        arduino::MbedSPI* _FRAM_SPI;
        uint8_t _FRAM_CS;
        
        uint8_t _FRAM_status = 0b00000000;
        bool _FRAM_WEL = false;
        bool _FRAM_BP0 = false;
        bool _FRAM_BP1 = false;
        
        void readStatus(){
            digitalWrite(this->_FRAM_CS, LOW);
            // read the status register
            this->_FRAM_SPI->transfer(RDSR);
            uint8_t status = this->_FRAM_SPI->transfer(0x00);
            this->_FRAM_status = status;
            // store the status register bits
            this->_FRAM_WEL = (status & 0b00000010) >> 1;
            this->_FRAM_BP0 = (status & 0b00000100) >> 2;
            this->_FRAM_BP1 = (status & 0b00001000) >> 3;
            digitalWrite(this->_FRAM_CS, HIGH);
        }

    public:
        FRAM(arduino::MbedSPI* FRAM_SPI, uint8_t FRAM_CS){
            this->_FRAM_SPI = FRAM_SPI;
            this->_FRAM_CS = FRAM_CS;
        }

        void init(){
            pinMode(this->_FRAM_CS, OUTPUT);
            this->_FRAM_SPI->begin();
            this->_FRAM_SPI->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
        }

        bool getWEL(){
            this->readStatus();
            return this->_FRAM_WEL;
        }

        bool getBP0(){
            this->readStatus();
            return this->_FRAM_BP0;
        }

        bool getBP1(){
            this->readStatus();
            return this->_FRAM_BP1;
        }

        uint8_t getStatus(){
            this->readStatus();
            return this->_FRAM_status;
        }

        void setWEL(bool WEL){
            digitalWrite(this->_FRAM_CS, LOW);
            if(WEL){
                this->_FRAM_SPI->transfer(this->WREN);
            }else{
                this->_FRAM_SPI->transfer(this->WRDI);
            }
            digitalWrite(this->_FRAM_CS, HIGH);
        }

        void setBP(bool BP0, bool BP1){
            setWEL(true);
            this->readStatus();
            digitalWrite(this->_FRAM_CS, LOW);
            this->_FRAM_SPI->transfer(this->WRSR);
            uint8_t status = this->_FRAM_status & 0b11110011;
            status |= BP0 << 2;
            status |= BP1 << 3;
            this->_FRAM_SPI->transfer(status);
            digitalWrite(this->_FRAM_CS, HIGH);
        }

        void write(uint16_t address, uint8_t data){
            setWEL(true);
            digitalWrite(this->_FRAM_CS, LOW);
            this->_FRAM_SPI->transfer(this->WRITE | ((address & 0b100000000) >> 5));
            this->_FRAM_SPI->transfer((uint8_t)(address & 0xFF));
            this->_FRAM_SPI->transfer(data);
            digitalWrite(this->_FRAM_CS, HIGH);
        }

        uint8_t read(uint16_t address){
            digitalWrite(this->_FRAM_CS, LOW);
            this->_FRAM_SPI->transfer(this->READ | ((address & 0b100000000) >> 5));
            this->_FRAM_SPI->transfer((uint8_t)(address & 0xFF));
            uint8_t data = this->_FRAM_SPI->transfer(0x00);
            digitalWrite(this->_FRAM_CS, HIGH);
            return data;
        }

        void write(uint16_t address, uint8_t* data, uint16_t length){
            setWEL(true);
            digitalWrite(this->_FRAM_CS, LOW);
            this->_FRAM_SPI->transfer(this->WRITE | ((address & 0b100000000) >> 5));
            this->_FRAM_SPI->transfer((uint8_t)(address & 0xFF));
            for(uint16_t i = 0; i < length; i++){
                this->_FRAM_SPI->transfer(data[i]);
            }
            digitalWrite(this->_FRAM_CS, HIGH);
        }

        void read(uint16_t address, uint8_t* data, uint16_t length){
            digitalWrite(this->_FRAM_CS, LOW);
            this->_FRAM_SPI->transfer(this->READ | ((address & 0b100000000) >> 5));
            this->_FRAM_SPI->transfer((uint8_t)(address & 0xFF));
            for(uint16_t i = 0; i < length; i++){
                data[i] = this->_FRAM_SPI->transfer(0x00);
            }
            digitalWrite(this->_FRAM_CS, HIGH);
        }

};