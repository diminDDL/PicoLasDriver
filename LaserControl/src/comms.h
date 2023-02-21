#ifndef COMMS_H
#define COMMS_H
#include <Arduino.h>

#define EOL "\r\n"                          // end of line characters

/*
The communications class is responsible for sending and receiving data.
It does everything from parsing the data to maintaining the heartbeat.
All of the variables that we send back and forth are stored here.
These variables are public so that the user can access them.
*/
class Communications{
    private:
        // command list
        const String getGlobalPulseCountCommand = "gpsc";       // string representation of the pulse count command
        const String getLocalPulseCountCommand = "gcpc";        // string representation of the local pulse count command
        const String setCurrentCommand = "scur";                // string representation of the set current command
        const String getCurrentCommand = "gcur";                // string representation of the get current command
        const String setMaxCurrentCommand = "smcu";             // string representation of the set max current command
        const String getMaxCurrentCommand = "gmcu";             // string representation of the get max current command
        const String setPulseDurationCommand = "spdu";          // string representation of the set pulse duration command
        const String getPulseDurationCommand = "gpdu";          // string representation of the get pulse duration command
        const String setPulseFrequencyCommand = "spfr";         // string representation of the set pulse frequency command
        const String getPulseFrequencyCommand = "gpfr";         // string representation of the get pulse frequency command
        const String enableOutputCommand = "enab";              // string representation of the enable output command
        const String lockCommand = "lock";                      // string representation of the lock command
        const String setAnalogModeCommand = "anmo";             // string representation of the analog mode command
        const String getModeCommand = "gmod";                   // string representation of the get mode command
        const String setGpioStateCommand = "stio";              // string representation of the set GPIO state command
        // internal variables
        UARTClass *serial;          // pointer to the serial port
        UARTClass *fwdPort;         // pointer to the serial port to forward data to
        uint32_t bdRate;            // baud rate of the serial port
        UARTClass::UARTModes mdReg; // mode register of the serial port
        char readBuff[64];          // array for the serial data
        char drivBuff[64];          // array for the serial data
        bool newData = false;       // flag to indicate that new data has been received
        char errorStr[3] = "00";    // string to hold the error code
    public:
        // data structure: 
        // get commands: <command>\n
        // return values: <value>\r\n<00>\r\n
        // set commands: <command> <value>\n
        // return values: <value>\r\n<00>\r\n
        // the \r\n are defined in EOL
        Communications(UARTClass *serial, UARTClass *forwardPort, const uint32_t baudRate, const UARTClass::UARTModes config);
        void init(void);
        void readSerial(void);
        void parseBuffer(void);
        void sendHeartbeat(void);
        void printErrorStr(bool commandType);
        void print_big_int(uint64_t value);

        bool valuesChanged = false;     // flag to indicate if the values have changed
        // struct with the data
        struct Data{
            uint64_t globalPulseCount = 0;                          // counts the total number of pulses sent do the driver
            uint64_t localPulseCount = 0;                           // counts the number of pulses sent to the driver since it was turned on
            float setCurrent = 0.0;                                 // current set by the user in Amperes
            float maxCurrent = 0.0;                                 // max current set by the user in Amperes
            uint32_t setPulseDuration = 0;                          // pulse duration set by the user (in microseconds)
            uint32_t setPulseFrequency = 0;                         // pulse frequency set by the user (in Hz)
            bool outputEnabled = false;                             // output enabled flag
            bool lockState = false;                                 // lock state of the driver and UI (locked = true, unlocked = false)
            bool analogMode = false;                                // analog mode flag
            byte gpioState = 0;                                     // state of the GPIO pins
        } data;
        bool error1 = false;        // Not implemented (should be used to read error states of the driver)
        bool error2 = false;
        
};

#endif