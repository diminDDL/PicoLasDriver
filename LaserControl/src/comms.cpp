#include <comms.h>
#include <Arduino.h>

// constructor
Communications::Communications(UARTClass *serial, UARTClass *forwardPort, const uint32_t baudRate, const UARTClass::UARTModes config){
    this->serial = serial;
    this->fwdPort = forwardPort;
    bdRate = baudRate;
    mdReg = config;
}

void Communications::init(void){
    // initialize the serial port
    serial->begin(bdRate, mdReg);
    // initialize the forward port
    fwdPort->begin(bdRate, mdReg);
}

void Communications::readSerial(void){
    // read the data from serial port and store it in the dest array
    uint8_t i = 0;
    uint8_t size = sizeof(readBuff);
    if (serial->available() >0){
        // clear the buffer
        memset(readBuff, 0, size);
    }
    while (serial->available() > 0) {
        readBuff[i] = (char)serial->read();
        i++;
        newData = true;
        if (i >= size) {
            break;
        }
    }
}

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
    serial->print(errorStr);
    serial->print(EOL);
    if (commandType){
        valuesChanged = true;
    }
}

void Communications::print_big_int(uint64_t value){
    if ( value >= 10 ){
        print_big_int(value / 10);
    }
    serial->print((uint32_t)(value % 10));
}

void Communications::parseBuffer(void){
    if(!newData){
        return;
    }
    newData = false;
    // check if there is a " " in the string
    char * pch;
    pch = strchr(readBuff, ' ');
    // if we are in digital mode forward the data to the driver and record the response
    if (!data.analogMode){
        // forward the data to the driver
        fwdPort->print(readBuff);
        // wait a bit for the response
        delay(50);
        // read the response from the driver
        uint8_t i = 0;
        while (fwdPort->available() > 0) {
            drivBuff[i] = (char)fwdPort->read();
            i++;
            if (i >= 32) {
                break;
            }
        }
    }
    // if the space is in the message that means it's a set command (get commands don't have spaces)
    if (pch != NULL){
        // split the string into command and value by the " " character, also remove all whitespace
        char * command = strtok(readBuff, " ");
        char * value = strtok(NULL, " ");

        // check if the command is known
        if (strcmp(command, setCurrentCommand.c_str()) == 0){
            // set the current
            data.setCurrent = atof(value);
            if (data.setCurrent > data.maxCurrent){
                data.setCurrent = data.maxCurrent;
            }
            serial->print(data.setCurrent);
            serial->print(EOL);
            // since printErrorStr is called every time we update the values it updates the values changed flag as well
            printErrorStr(true);
        } else if (strcmp(command, setMaxCurrentCommand.c_str()) == 0){
            // set the max current
            data.maxCurrent = atof(value);
            if(data.maxCurrent < data.setCurrent){
                data.setCurrent = data.maxCurrent;
            }
            serial->print(data.maxCurrent);
            serial->print(EOL);
            printErrorStr(true);
        } else if (strcmp(command, setPulseDurationCommand.c_str()) == 0){
            // set the pulse duration
            data.setPulseDuration = atol(value);
            serial->print(data.setPulseDuration);
            serial->print(EOL);
            printErrorStr(true);
        } else if (strcmp(command, setPulseFrequencyCommand.c_str()) == 0){
            // set the pulse frequency
            data.setPulseFrequency = atol(value);
            serial->print(data.setPulseFrequency);
            serial->print(EOL);
            printErrorStr(true);
        } else if (strcmp(command, enableOutputCommand.c_str()) == 0){
            // enable the output, 1 = enable, 0 = disable if the string is invalid it will still return 0 thus disabling the output if the host sends an invalid values
            if (atoi(value) == 1){
                data.outputEnabled = true;
            } else if (atoi(value) == 0){
                data.outputEnabled = false;
            } else {
                data.outputEnabled = false;
            }
            serial->print(data.outputEnabled, DEC);
            serial->print(EOL);
            printErrorStr(true);
        } else if (strcmp(command, lockCommand.c_str()) == 0){
            // lock the driver, 1 = lock, 0 = unlock if the string is invalid it will still return 0 thus disabling the output if the host sends an invalid values
            if (atoi(value) == 1){
                data.lockState = true;
            } else if (atoi(value) == 0){
                data.lockState = false;
            } else {
                data.lockState = false;
            }
            serial->print(data.lockState, DEC);
            serial->print(EOL);
            printErrorStr(true);
        } else if (strcmp(command, setAnalogModeCommand.c_str()) == 0){
            // set the driver to analog mode
            if (atoi(value) == 1){
                data.analogMode = true;
            } else if (atoi(value) == 0){
                data.analogMode = false;
            } else {
                data.analogMode = false;
            }
            serial->print(data.analogMode, DEC);
            serial->print(EOL);
            printErrorStr(true);
        } else if(strcmp(command, setGpioStateCommand.c_str()) == 0){ 
            // set the GPIO state 
            data.gpioState = (atoi(value) > 0) && (atoi(value) < 256) ? atoi(value) : 0;
            serial->print(data.gpioState);
            serial->print(EOL);
            printErrorStr(true);

            serial->println(data.gpioState, BIN);
        }else {
            // unknown command
            serial->print("UC");
            serial->print(EOL);
            printErrorStr();
        }
    }else{
        // if it's a get command perhaps
        // remove all whitespace and new line characters
        char * command = strtok(readBuff, " \r\n");
        // check if the command is known
        if (strcmp(readBuff, getGlobalPulseCountCommand.c_str()) == 0){
            // get the global pulse count
            print_big_int(data.globalPulseCount);
            serial->print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getLocalPulseCountCommand.c_str()) == 0){
            // get the local pulse count
            print_big_int(data.localPulseCount);
            serial->print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getCurrentCommand.c_str()) == 0){
            // get the current
            serial->print(data.setCurrent);
            serial->print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getMaxCurrentCommand.c_str()) == 0){
            // get the max current
            serial->print(data.setCurrent);
            serial->print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getPulseDurationCommand.c_str()) == 0){
            // get the pulse duration
            serial->print(data.setPulseDuration);
            serial->print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getPulseFrequencyCommand.c_str()) == 0){
            // get the pulse frequency
            serial->print(data.setPulseFrequency);
            serial->print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getModeCommand.c_str()) == 0){
            // get the mode
            serial->print(data.analogMode, DEC);
            serial->print(EOL);
            printErrorStr();
        } else {
            // unknown command
            serial->print("UC");
            serial->print(EOL);
            printErrorStr();
        }
    }
    ///// remove later
    // TODO test cahnges
    serial->println();
    // print all the new values
    serial->print("Current: ");
    serial->println(data.setCurrent);
    serial->print("Max current: ");
    serial->println(data.maxCurrent);
    serial->print("Pulse duration: ");
    print_big_int(data.setPulseDuration);
    serial->println();
    serial->print("Pulse frequency: ");
    serial->println(data.setPulseFrequency);
    serial->print("Output enabled: ");
    serial->println(data.outputEnabled);
    serial->print("Lock state: ");
    serial->println(data.lockState);
    serial->print("Analog mode: ");
    serial->println(data.analogMode);
    serial->print("GPIO state: ");
    serial->println(data.gpioState);
    serial->print("Changed: ");
    serial->println(valuesChanged);
    /////
}



