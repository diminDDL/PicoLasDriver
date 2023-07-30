#include <comms.hpp>
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdio.h"
#include "tusb.h"
#include <string.h>

/*
* Constructor
* @param serial pointer to the serial port object
* @param forwardPort pointer to the port where the data will be forwarded
* @param baudRate baud rate of the serial port
* @param config configuration of the serial port, see UARTClass::UARTModes
*/
Communications::Communications(){

}

/*
* Initialize the serial ports
*/
void Communications::init(void){
    // something
}

/*
* Read the serial port into the buffer
*/
void Communications::readSerial(void){
    // read the data from serial port and store it in the dest array
    uint32_t i = 0;
    uint32_t size = sizeof(readBuff);
    if (tud_cdc_available()){
        // clear the buffer
        memset(readBuff, 0, size);
    }
    while (tud_cdc_available()) {
        readBuff[i] = (char)tud_cdc_read_char();
        i++;
        newData = true;
        if (i >= size) {
            break;
        }
    }
    // update internal variables
    updateValues();
}

/*
* Print the error string, and update the valuesChanged flag if it is a set command
* @param commandType true if it is a set command, false if it is a get command
*/
void Communications::printErrorStr(bool commandType = false){
    // commandType = true - set command
    // commandType = false - get command
    // set the error string
    if (error1){
        errorStr[0] = '1';
    } else {
        errorStr[0] = '0';
    }
    if (error2){
        errorStr[1] = '1';
    } else {
        errorStr[1] = '0';
    }
    // print the error string
    printf("%s%s", errorStr, EOL);
    if(estopFlag){
        printf("%s%s", estop, EOL);
    }

    if (commandType){
        valuesChanged = true;
    }
}

/*
* Used to print a uint64_t value
* @param value the value to be printed
*/
void Communications::print_big_int(uint64_t value){
    if ( value >= 10 ){
        print_big_int(value / 10);
    }
    printf("%d", (uint32_t)(value % 10));
}

/*
* Update the values in the data struct
*/

void Communications::updateValues(void){
    data.localPulseCount = data.globalPulseCount - data.initPulseCount;
}

/*
* Parse the buffer and update the data struct
*/
void Communications::parseBuffer(void){
    // TODO remove this
    // printf("%s", readBuff);
    if(!newData){
        return;
    }

    newData = false;

    // if the message equals the magic string, then respond with the magic string backwards
    if (strcmp(readBuff, magicString) == 0){
        // print the reversed magic string
        for(int i = strlen(magicString) - 1; i >= 0; i--){
            printf("%c", magicString[i]);
        }
        // clear the buffer
        memset(readBuff, 0, sizeof(readBuff));
        return;
    }

    // check if there is a " " in the string
    char * pch;
    pch = strchr(readBuff, ' ');
    // if we are in digital mode forward the data to the driver and record the response
    if (!data.analogMode){
        // forward the data to the driver
        // TODO implement the digital mode
        // fwdPort->print(readBuff);
        // wait a bit for the response
        // delay(50);
        // read the response from the driver
        // uint8_t i = 0;
        // while (fwdPort->available() > 0) {
        //     drivBuff[i] = (char)fwdPort->read();
        //     i++;
        //     if (i >= 32) {
        //         break;
        //     }
        // }
    }
    // if the space is in the message that means it's a set command (get commands don't have spaces)
    if (pch != NULL){
        // split the string into command and value by the " " character, also remove all whitespace
        char * command = strtok(readBuff, " ");
        char * value = strtok(NULL, " ");

        // check if the command is known
        if (strcmp(command, setCurrentCommand) == 0){
            // set the current
            data.setCurrent = atof(value);
            if (data.setCurrent > data.maxCurrent){
                data.setCurrent = data.maxCurrent;
            }
            printf("%f%s", data.setCurrent, EOL);
            // since printErrorStr is called every time we update the values it updates the values changed flag as well
            printErrorStr(true);
        } else if (strcmp(command, setMaxCurrentCommand) == 0){
            // set the max current
            data.maxCurrent = atof(value);
            if(data.maxCurrent < data.setCurrent){
                data.setCurrent = data.maxCurrent;
            }
            printf("%f%s", data.maxCurrent, EOL);
            printErrorStr(true);
        } else if (strcmp(command, setPulseDurationCommand) == 0){
            // set the pulse duration
            uint32_t val = atol(value);
            if (val == 0){
                val = 1;
            }
            uint32_t pwm_period = (1000000 / (data.setPulseFrequency));
            if(val >= pwm_period){
                val = pwm_period - 1;
            }
            data.setPulseDuration = val;
            printf("%lu%s", data.setPulseDuration, EOL);
            printErrorStr(true);
        } else if (strcmp(command, setPulseFrequencyCommand) == 0){
            // set the pulse frequency
            uint32_t val = atol(value);
            if (val == 0){
                val = 1;
            }
            data.setPulseFrequency = val;
            uint32_t pwm_period = (1000000 / (data.setPulseFrequency));
            if(data.setPulseDuration >= pwm_period){
                data.setPulseDuration = pwm_period - 1;
            }
            printf("%lu%s", data.setPulseFrequency, EOL);
            printErrorStr(true);
        } else if (strcmp(command, enableOutputCommand) == 0){
            // enable the output, 1 = enable, 0 = disable if the string is invalid it will still return 0 thus disabling the output if the host sends an invalid values
            if (atoi(value) == 1){
                data.outputEnabled = true;
            } else if (atoi(value) == 0){
                data.outputEnabled = false;
            } else {
                data.outputEnabled = false;
            }
            printf("%d%s", data.outputEnabled, EOL);
            printErrorStr(true);
        } else if (strcmp(command, lockCommand) == 0){
            // lock the driver, 1 = lock, 0 = unlock if the string is invalid it will not change the lock state
            if (atoi(value) == 1){
                data.lockState = true;
            } else if (atoi(value) == 0){
                data.lockState = false;
            }
            printf("%d%s", data.lockState, EOL);
            printErrorStr(true);
        } else if (strcmp(command, setAnalogModeCommand) == 0){
            // set the driver to analog mode
            if (atoi(value) == 1){
                data.analogMode = true;
            } else if (atoi(value) == 0){
                data.analogMode = false;
            } else {
                data.analogMode = false;
            }
            printf("%d%s", data.analogMode, EOL);
            printErrorStr(true);
        } else if(strcmp(command, setGpioStateCommand) == 0){ 
            // set the GPIO state 
            data.gpioState = (atoi(value) > 0) && (atoi(value) < 256) ? atoi(value) : 0;
            printf("%d%s", data.gpioState, EOL);
            printErrorStr(true);

        } else if(strcmp(command, setPulseMode) == 0){
            // 0 - continuous, 1 - single pulse
            if (atoi(value) == 1){
                data.pulseMode = 1;
            } else if (atoi(value) == 0){
                data.pulseMode = 0;
            }
            printf("%d%s", data.pulseMode, EOL);
            printErrorStr(true);
        } else if (strcmp(command, countReset) == 0){
            // 1 - reset, 0 - do nothing
            if (atoi(value) == 1){
                data.globalPulseCount = 0;
                data.initPulseCount = 0;
            }
            print_big_int(data.globalPulseCount);
            printf("%s", EOL);
            printErrorStr(true);
        }else {
            // unknown command
            printf("UC%s", EOL);
            printErrorStr();
        }
    }else{
        // if it's a get command perhaps
        // remove all whitespace and new line characters
        char * command = strtok(readBuff, " \r\n");
        // check if the command is known
        if (strcmp(readBuff, getGlobalPulseCountCommand) == 0){
            // get the global pulse count
            print_big_int(data.globalPulseCount);
            printf("%s", EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getLocalPulseCountCommand) == 0){
            updateValues();
            // get the local pulse count
            print_big_int(data.localPulseCount);
            printf("%s", EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getCurrentCommand) == 0){
            // get the current
            printf("%f%s", data.setCurrent, EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getMaxCurrentCommand) == 0){
            // get the max current
            printf("%f%s", data.maxCurrent, EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getPulseDurationCommand) == 0){
            // get the pulse duration
            printf("%lu%s", data.setPulseDuration, EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getPulseFrequencyCommand) == 0){
            // get the pulse frequency
            printf("%lu%s", data.setPulseFrequency, EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getModeCommand) == 0){
            // get the mode
            printf("%d%s", data.analogMode, EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getAdcCommand) == 0){
            // get the adc value
            printf("%d%s", data.adcValue, EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getPulseMode) == 0){
            // print the pulse mode 
            printf("%d%s", data.pulseMode, EOL);
            printErrorStr();
        } else if (strcmp(readBuff, eraseMemory) == 0){
            // erase the memory
            eraseFlag = true;
            printf("EM%s", EOL);
            printErrorStr();
        } else {
            // unknown command
            printf("UC%s", EOL);
            printErrorStr();
        }
    }

    // clear the buffer
    memset(readBuff, 0, sizeof(readBuff));

    ///// remove later
    // TODO test cahnges
    // printf("\n");
    // print all the new values
    // printf("Current: %f\n", data.setCurrent);
    // printf("Max current: %f\n", data.maxCurrent);
    // printf("Pulse duration: ");
    // print_big_int(data.setPulseDuration);
    // printf("\n");
    // printf("Pulse frequency: %lu\n", data.setPulseFrequency);
    // printf("Output enabled: %d\n", data.outputEnabled);
    // printf("Lock state: %d\n", data.lockState);
    // printf("Analog mode: %d\n", data.analogMode);
    // printf("GPIO state: %d\n", data.gpioState);
    // printf("Pulse mode: %d\n", data.pulseMode);
    // printf("Changed: %d\n", valuesChanged);
    /////
}
