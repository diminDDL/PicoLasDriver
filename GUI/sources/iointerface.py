import sys
import glob
import serial
from time import sleep


class IOInterface:
    def __init__(self, debug: bool, config: dict, driver: str):
        # if config is empty throw an error
        if not config:
            raise ValueError("Config is empty")
        self.conf = config
        self.debug = debug
        self.driver = driver
        self.baudrate = self.conf[self.driver]["protocol"]["connection"]["baudrate"]
        self.bits = self.conf[self.driver]["protocol"]["connection"]["bits"]
        self.parity = self.conf[self.driver]["protocol"]["connection"]["parity"]
        self.stopbits = self.conf[self.driver]["protocol"]["connection"]["stopbits"]
        # print debug info
        if self.debug:
            print("Driver model: " + self.driver)
            print("Baudrate: " + str(self.baudrate))
            print("Bits: " + str(self.bits))
            print("Parity: " + self.parity)
            print("Stopbits: " + str(self.stopbits))

        # scan for all serial ports
        ports = self.get_serial_ports()
        if self.debug:
            print("Found ports: " + str(ports))
        # connect to each port using the baudrate and timeout specified in the config file and send the magic number
        for port in ports:
            #try:
            if True:
                #s.write(bytearray.fromhex(self.conf[self.driver]["protocol"]["connection"]["magic"].strip("0x")))
                #print(bytearray.fromhex(self.conf[self.driver]["protocol"]["connection"]["magic"].strip("0x")))
                # write "test" to serial
                s = serial.Serial(port, 115200, timeout=1, stopbits=serial.STOPBITS_ONE, parity=serial.PARITY_NONE, bytesize=serial.EIGHTBITS)
                s.write(b"testtesttesttest\n\r")
                sleep(0.1)
                res = s.read(2)
                sleep(0.1)
                if self.debug:
                    print("Received: " + str(res))
                s.close()
            #except serial.SerialException:
            #    print("Error connecting to port: " + port)

    def execute(self, command: str, args):
        # execute a command
        if self.debug:
            print("Executing command: " + command)

    def get_serial_ports(self):
        """ Lists serial port names

            :raises EnvironmentError:
                On unsupported or unknown platforms
            :returns:
                A list of the serial ports available on the system
        """
        if sys.platform.startswith('win'):
            ports = ['COM%s' % (i + 1) for i in range(256)]
        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
            # this excludes your current terminal "/dev/tty"
            ports = glob.glob('/dev/tty[A-Za-z]*')
        elif sys.platform.startswith('darwin'):
            ports = glob.glob('/dev/tty.*')
        else:
            raise EnvironmentError('Unsupported platform')

        result = []
        for port in ports:
            try:
                s = serial.Serial(port)
                s.close()
                result.append(port)
            except (OSError, serial.SerialException):
                pass
        return result