#ifndef FRAM_H
#define FRAM_H

#include "hardware/spi.h"
#include "pico/stdlib.h"

#define FRAM_SPI_BAUD 1000000               // FRAM SPI baud rate

class FRAM{
    private:
        uint8_t WREN  = 0b00000110;     // Set write enable latch
        uint8_t WRDI  = 0b00000100;     // Write disable
        uint8_t RDSR  = 0b00000101;     // Read status register
        uint8_t WRSR  = 0b00000001;     // Write status register
        uint8_t READ  = 0b00000011;     // Read memory data
        uint8_t WRITE = 0b00000010;     // Write memory data

        spi_inst_t* _FRAM_SPI;
        uint8_t _FRAM_CS;
        
        uint8_t _FRAM_status = 0b00000000;
        bool _FRAM_WEL = false;
        bool _FRAM_BP0 = false;
        bool _FRAM_BP1 = false;
        
        void readStatus(){
            gpio_put(this->_FRAM_CS, false);
            // read the status register
            spi_write_blocking(this->_FRAM_SPI, &RDSR, 1);
            uint8_t status;
            spi_read_blocking(this->_FRAM_SPI, 0x00, &status, 1);

            this->_FRAM_status = status;
            // store the status register bits
            this->_FRAM_WEL = (status & 0b00000010) >> 1;
            this->_FRAM_BP0 = (status & 0b00000100) >> 2;
            this->_FRAM_BP1 = (status & 0b00001000) >> 3;
            gpio_put(this->_FRAM_CS, true);
        }

    public:
        FRAM(spi_inst_t* FRAM_SPI, uint8_t FRAM_CS){
            this->_FRAM_SPI = FRAM_SPI;
            this->_FRAM_CS = FRAM_CS;
        }

        void init(){
            gpio_init(this->_FRAM_CS);
            gpio_set_dir(this->_FRAM_CS, GPIO_OUT);
            gpio_put(this->_FRAM_CS, true);

            spi_init(this->_FRAM_SPI, FRAM_SPI_BAUD);
            spi_set_format(this->_FRAM_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
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
            gpio_put(this->_FRAM_CS, false);
            if(WEL){
                spi_write_blocking(this->_FRAM_SPI, &this->WREN, 1);
            }else{
                spi_write_blocking(this->_FRAM_SPI, &this->WRDI, 1);
            }
            gpio_put(this->_FRAM_CS, true);
        }

        void setBP(bool BP0, bool BP1){
            setWEL(true);
            this->readStatus();
            gpio_put(this->_FRAM_CS, false);
            spi_write_blocking(this->_FRAM_SPI, &this->WRSR, 1);
            uint8_t status = this->_FRAM_status & 0b11110011;
            status |= BP0 << 2;
            status |= BP1 << 3;
            spi_write_blocking(this->_FRAM_SPI, &status, 1);
            gpio_put(this->_FRAM_CS, true);
        }

        void write(uint16_t address, uint8_t data){
            setWEL(true);
            gpio_put(this->_FRAM_CS, false);
            uint8_t buffer[3];
            buffer[0] = this->WRITE | ((address & 0b100000000) >> 5);
            buffer[1] = (uint8_t)(address & 0xFF);
            buffer[2] = data;
            spi_write_blocking(this->_FRAM_SPI, buffer, 3);
            gpio_put(this->_FRAM_CS, true);
        }

        uint8_t read(uint16_t address){
            gpio_put(this->_FRAM_CS, false);
            uint8_t buffer[2];
            buffer[0] = this->READ | ((address & 0b100000000) >> 5);
            buffer[1] = (uint8_t)(address & 0xFF);
            spi_write_blocking(this->_FRAM_SPI, buffer, 2);
            uint8_t data;
            spi_read_blocking(this->_FRAM_SPI, 0x00, &data, 1);
            gpio_put(this->_FRAM_CS, true);
            return data;
        }

        void write(uint16_t address, uint8_t* data, uint16_t length){
            setWEL(true);
            gpio_put(this->_FRAM_CS, false);
            uint8_t buffer[2];
            buffer[0] = this->WRITE | ((address & 0b100000000) >> 5);
            buffer[1] = (uint8_t)(address & 0xFF);
            spi_write_blocking(this->_FRAM_SPI, buffer, 2);
            spi_write_blocking(this->_FRAM_SPI, data, length);
            gpio_put(this->_FRAM_CS, true);
        }

        void read(uint16_t address, uint8_t* data, uint16_t length){
            gpio_put(this->_FRAM_CS, false);
            uint8_t buffer[2];
            buffer[0] = this->READ | ((address & 0b100000000) >> 5);
            buffer[1] = (uint8_t)(address & 0xFF);
            spi_write_blocking(this->_FRAM_SPI, buffer, 2);
            spi_read_blocking(this->_FRAM_SPI, 0x00, data, length);
            gpio_put(this->_FRAM_CS, true);
        }

};

#endif // FRAM_H