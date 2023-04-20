#include <comms.h>
#include <Arduino.h>

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
    if (Serial.available() >0){
        // clear the buffer
        memset(readBuff, 0, size);
    }
    while (Serial.available() > 0) {
        readBuff[i] = (char)Serial.read();
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
    Serial.print(errorStr);
    Serial.print(EOL);
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
    Serial.print((uint32_t)(value % 10));
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
    Serial.print(readBuff);

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
        if (strcmp(command, setCurrentCommand.c_str()) == 0){
            // set the current
            data.setCurrent = atof(value);
            if (data.setCurrent > data.maxCurrent){
                data.setCurrent = data.maxCurrent;
            }
            Serial.print(data.setCurrent);
            Serial.print(EOL);
            // since printErrorStr is called every time we update the values it updates the values changed flag as well
            printErrorStr(true);
        } else if (strcmp(command, setMaxCurrentCommand.c_str()) == 0){
            // set the max current
            data.maxCurrent = atof(value);
            if(data.maxCurrent < data.setCurrent){
                data.setCurrent = data.maxCurrent;
            }
            Serial.print(data.maxCurrent);
            Serial.print(EOL);
            printErrorStr(true);
        } else if (strcmp(command, setPulseDurationCommand.c_str()) == 0){
            // set the pulse duration
            data.setPulseDuration = atol(value);
            Serial.print(data.setPulseDuration);
            Serial.print(EOL);
            printErrorStr(true);
        } else if (strcmp(command, setPulseFrequencyCommand.c_str()) == 0){
            // set the pulse frequency
            data.setPulseFrequency = atol(value);
            Serial.print(data.setPulseFrequency);
            Serial.print(EOL);
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
            Serial.print(data.outputEnabled, DEC);
            Serial.print(EOL);
            printErrorStr(true);
        } else if (strcmp(command, lockCommand.c_str()) == 0){
            // lock the driver, 1 = lock, 0 = unlock if the string is invalid it will not change the lock state
            if (atoi(value) == 1){
                data.lockState = true;
            } else if (atoi(value) == 0){
                data.lockState = false;
            }
            Serial.print(data.lockState, DEC);
            Serial.print(EOL);
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
            Serial.print(data.analogMode, DEC);
            Serial.print(EOL);
            printErrorStr(true);
        } else if(strcmp(command, setGpioStateCommand.c_str()) == 0){ 
            // set the GPIO state 
            data.gpioState = (atoi(value) > 0) && (atoi(value) < 256) ? atoi(value) : 0;
            Serial.print(data.gpioState, DEC);
            Serial.print(EOL);
            printErrorStr(true);

            Serial.println(data.gpioState, BIN);
        } else if(strcmp(command, setPulseMode.c_str()) == 0){
            if (atoi(value) == 1){
                data.pulseMode = 1;
            } else if (atoi(value) == 0){
                data.pulseMode = 0;
            }
            Serial.print(data.pulseMode, DEC);
            Serial.print(EOL);
            printErrorStr(true);
        } else {
            // unknown command
            Serial.print("UC");
            Serial.print(EOL);
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
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getLocalPulseCountCommand.c_str()) == 0){
            updateValues();
            // get the local pulse count
            print_big_int(data.localPulseCount);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getCurrentCommand.c_str()) == 0){
            // get the current
            Serial.print(data.setCurrent);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getMaxCurrentCommand.c_str()) == 0){
            // get the max current
            Serial.print(data.setCurrent);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getPulseDurationCommand.c_str()) == 0){
            // get the pulse duration
            Serial.print(data.setPulseDuration);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getPulseFrequencyCommand.c_str()) == 0){
            // get the pulse frequency
            Serial.print(data.setPulseFrequency);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getModeCommand.c_str()) == 0){
            // get the mode
            Serial.print(data.analogMode, DEC);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getAdcCommand.c_str()) == 0){
            // get the adc value
            Serial.print(data.adcValue);
            Serial.print(EOL);
            printErrorStr();
        } else if (strcmp(readBuff, getPulseMode.c_str()) == 0){
            // print the pulse mode
            Serial.print(data.pulseMode);
            Serial.print(EOL); 
        } else {
            // unknown command
            Serial.print("UC");
            Serial.print(EOL);
            printErrorStr();
        }
    }

    // clear the buffer
    memset(readBuff, 0, sizeof(readBuff));

    ///// remove later
    // TODO test cahnges
    Serial.println();
    // print all the new values
    Serial.print("Current: ");
    Serial.println(data.setCurrent);
    Serial.print("Max current: ");
    Serial.println(data.maxCurrent);
    Serial.print("Pulse duration: ");
    print_big_int(data.setPulseDuration);
    Serial.println();
    Serial.print("Pulse frequency: ");
    Serial.println(data.setPulseFrequency);
    Serial.print("Output enabled: ");
    Serial.println(data.outputEnabled);
    Serial.print("Lock state: ");
    Serial.println(data.lockState);
    Serial.print("Analog mode: ");
    Serial.println(data.analogMode);
    Serial.print("GPIO state: ");
    Serial.println(data.gpioState);
    Serial.print("Pulse mode: ");
    Serial.println(data.pulseMode);
    Serial.print("Changed: ");
    Serial.println(valuesChanged);
    /////
}



