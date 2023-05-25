#define SERIAL_BAUD_RATE 115200             // NOT USED
#define SERIAL_MODE SERIAL_8E1              // NOT USED
#define FORWARD_PORT NULL                   // NOT USED (when in digital mode all the communication will be forwarded to this port)

#define INTERRUPT_PIN 18                    // interrupt pin for the trigger
#define ESTOP_PIN 19                        // emergency stop pin, active low
#define EN_PIN 20                           // enable pin, active high
#define PULSE_PIN 14                        // pin used to generate the pulses
#define PULSE_COUNT_PIN 15                  // pin used to count the pulses
#define GPIO_BASE_PIN 10                    // base pin for the GPIO outputs
#define GPIO_NUM 4                          // number of GPIO pins
#define OE_LED 8                            // LED indicating wether the output is enabled
#define TRIG_LED 9                          // LED indicating Internal/external triggering
#define E_STOP_LED 21                       // LED indicating that the e-stop is halting the operation of the board
#define FAULT_LED 22                        // LED indicating an internal fault
#define MAX_DAC_VOLTAGE 1.5                 // maximum voltage of the DAC
#define DAC_VREF 3.3                        // DAC reference voltage
#define DAC_VDIV 2                          // DAC voltage divider ratio 2 means we are using a 1:2 divider aka the output is half the input
#define ADC_PIN_PD 27                       // pin used to read the photodiode
#define ADC_MUX_PD 1                        // ADC mux channel for photodiode

#define FRAM_SPI_SCLK 2                     // FRAM SPI clock pin
#define FRAM_SPI_MOSI 3                     // FRAM SPI MOSI pin
#define FRAM_SPI_MISO 4                     // FRAM SPI MISO pin
#define FRAM_SPI_CS 5                       // FRAM SPI CS pin
#define FRAM_SPI spi0                       // FRAM SPI instance

