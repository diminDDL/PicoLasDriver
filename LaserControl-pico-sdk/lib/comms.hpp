#ifndef COMMS_H
#define COMMS_H

#define EOL "\r\n"                          // end of line characters

#include <stdio.h>
#include "pico/stdio.h"

/*
The communications class is responsible for sending and receiving data.
It does everything from parsing the data to maintaining the heartbeat.
All of the variables that we send back and forth are stored here.
These variables are public so that the user can access them.
*/

/*
* Constructor
* @param serial - pointer to the serial port object
* @param forwardPort - pointer to the port where the data will be forwarded
* @param baudRate - baud rate of the serial port
* @param config - configuration of the serial port, see UARTClass::UARTModes
*/
class Communications{
    private:
        // TODO double check the command list in the json
        // command list
        const char magicString[8] = "picoBOB";                   // string representation of the magic string
        const char getGlobalPulseCountCommand[5] = "gpsc";       // string representation of the pulse count command
        const char getLocalPulseCountCommand[5] = "gcpc";        // string representation of the local pulse count command
        const char setCurrentCommand[5] = "scur";                // string representation of the set current command
        const char getCurrentCommand[5] = "gcur";                // string representation of the get current command
        const char setMaxCurrentCommand[5] = "smcu";             // string representation of the set max current command
        const char getMaxCurrentCommand[5] = "gmcu";             // string representation of the get max current command
        const char setPulseDurationCommand[5] = "spdu";          // string representation of the set pulse duration command
        const char getPulseDurationCommand[5] = "gpdu";          // string representation of the get pulse duration command
        const char setPulseFrequencyCommand[5] = "spfr";         // string representation of the set pulse frequency command
        const char getPulseFrequencyCommand[5] = "gpfr";         // string representation of the get pulse frequency command
        const char enableOutputCommand[5] = "enab";              // string representation of the enable output command
        const char lockCommand[5] = "lock";                      // string representation of the lock command
        const char setAnalogModeCommand[5] = "anmo";             // string representation of the analog mode command
        const char getModeCommand[5] = "gmod";                   // string representation of the get mode command
        const char setGpioStateCommand[5] = "stio";              // string representation of the set GPIO state command
        const char getAdcCommand[5] = "gadc";                    // string representation of the get ADC command
        const char setPulseMode[5] = "spmo";                     // string representation of the set pulse mode command
        const char getPulseMode[5] = "gpmo";                     // string representation of the get pulse mode command
        const char eraseMemory[5] = "eras";                      // string representation of the erase memory command
        const char countReset [5] = "cres";                        // string representation of the count reset command
        // internal variables
        char readBuff[128];          // array for the serial data
        char drivBuff[128];          // array for the serial data
        bool newData = false;       // flag to indicate that new data has been received
        char errorStr[3] = "00";    // string to hold the error code
    public:
        // data structure: 
        // get commands: <command>\n
        // return values: <value>\r\n<00>\r\n
        // set commands: <command> <value>\n
        // return values: <value>\r\n<00>\r\n
        // the \r\n are defined in EOL
        Communications();
        void init(void);
        void readSerial(void);
        void parseBuffer(void);
        void sendHeartbeat(void);
        void printErrorStr(bool commandType);
        void print_big_int(uint64_t value);
        void updateValues(void);

        bool valuesChanged = false;     // flag to indicate if the values have changed
        bool eraseFlag = false;         // flag to indicate if the memory should be erased
        // struct with the data
        struct Data{
            uint64_t globalPulseCount = 0;                          // counts the total number of pulses sent do the driver
            uint64_t initPulseCount = 0;                            // initial pulse count
            uint64_t localPulseCount = 0;                           // counts the number of pulses sent to the driver since it was turned on
            float setCurrent = 0.0;                                 // current set by the user in Amperes
            float maxCurrent = 0.0;                                 // max current set by the user in Amperes
            uint32_t setPulseDuration = 0;                          // pulse duration set by the user (in microseconds)
            uint32_t setPulseFrequency = 0;                         // pulse frequency set by the user (in Hz)
            bool outputEnabled = false;                             // output enabled flag
            bool lockState = false;                                 // lock state of the driver and UI (locked = true, unlocked = false)
            bool analogMode = false;                                // analog mode flag
            uint16_t adcValue = 0;                                  // has the latest ADC value
            uint8_t gpioState = 0;                                  // state of the GPIO pins
            uint8_t pulseMode = 0;                                  // pulse mode, 0 - continuous, 1 - single pulse
        } data;
        bool error1 = false;        // Not implemented (should be used to read error states of the driver)
        bool error2 = false;        // Used to indicate critical errors (driver halted, etc.)
        
};

#endif
