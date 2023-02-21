#ifndef COMMS_H
#define COMMS_H
#include <Arduino.h>

/*
The communications class is responsible for sending and receiving data.
It does everything from parsing the data to maintaining the heartbeat.
All of the variables that we send back and forth are stored here.
These variables are public so that the user can access them.
*/
class Communications{
    // variables go here
    private:
        // private variables go here
        struct comms_stings
        {
            
        };
    public:
        Communications(UARTClass *serial);
        void init();
        void send();
        void receive();
        void update();
        void print();
};

#endif