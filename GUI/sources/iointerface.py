import sys
import glob
import serial
from time import sleep


class IOInterface:
    def __init__(self, debug: bool, config: dict, driver: str, platform: str, loop):
        # if config is empty throw an error
        if not config:
            raise ValueError("Config is empty")
        self.conf = config
        self.debug = debug
        self.driver = driver
        self.platform = platform
        if self.conf[self.driver]["protocol"]["connection"]["type"] == "serial":
            self.baudrate = int(self.conf[self.driver]["protocol"]["connection"]["baudrate"])
            self.bits = int(self.conf[self.driver]["protocol"]["connection"]["bits"])
            self.parity = self.conf[self.driver]["protocol"]["connection"]["parity"]
            self.stopbits = int(self.conf[self.driver]["protocol"]["connection"]["stopbits"])

        self.loop = loop
        self.loop.run_in_executor(None, self.init_comms)

        self.commandQueue = [] # a string command queue that will be executed in order

    def init_comms(self):
        # scan for all serial ports
        ports = self.get_serial_ports()
        if self.debug:
            print("Found ports: " + str(ports))
        # connect to each port using the baudrate and timeout specified in the config file and send the magic number
        for port in ports:
            try:
                match self.conf[self.driver]["protocol"]["name"]:
                    case "RS232":
                        match self.bits:
                            case 5:
                                self.bits = serial.FIVEBITS
                            case 6:
                                self.bits = serial.SIXBITS
                            case 7:
                                self.bits = serial.SEVENBITS
                            case 8:
                                self.bits = serial.EIGHTBITS
                            case _:
                                raise ValueError("Invalid bits value")
                        match self.parity:
                            case "N":
                                self.parity = serial.PARITY_NONE
                            case "E":
                                self.parity = serial.PARITY_EVEN
                            case "O":
                                self.parity = serial.PARITY_ODD
                            case _:
                                raise ValueError("Invalid parity value")
                        match self.stopbits:
                            case 1:
                                self.stopbits = serial.STOPBITS_ONE
                            case 2:
                                self.stopbits = serial.STOPBITS_TWO
                            case _:
                                raise ValueError("Invalid stopbits value")
                        # print debug info
                        if self.debug:
                            print("Driver model: " + self.driver)
                            print("Baudrate: " + str(self.baudrate))
                            print("Bits: " + str(self.bits))
                            print("Parity: " + str(self.parity))
                            print("Stopbits: " + str(self.stopbits))
                        s = serial.Serial(port, self.baudrate, timeout=1, stopbits=self.stopbits, parity=self.parity, bytesize=self.bits)
                        sleep(3)
                        # s.write(b"testtesttesttest\n\r")
                        magic = int(self.conf[self.driver]["protocol"]["connection"]["magic"][2::], 16)
                        print(magic)
                        s.write("")
                        sleep(0.1)
                        res = s.read(10)
                        sleep(0.1)
                        if self.debug:
                            print("Received: " + str(res))
                        s.close()
                    case "RS485":
                        print("Not implemented")
                    case "BOB":
                        if self.conf[self.driver]["protocol"]["connection"]["type"] == "serial":
                            match self.bits:
                                case 5:
                                    self.bits = serial.FIVEBITS
                                case 6:
                                    self.bits = serial.SIXBITS
                                case 7:
                                    self.bits = serial.SEVENBITS
                                case 8:
                                    self.bits = serial.EIGHTBITS
                                case _:
                                    raise ValueError("Invalid bits value")
                            match self.parity:
                                case "N":
                                    self.parity = serial.PARITY_NONE
                                case "E":
                                    self.parity = serial.PARITY_EVEN
                                case "O":
                                    self.parity = serial.PARITY_ODD
                                case _:
                                    raise ValueError("Invalid parity value")
                            match self.stopbits:
                                case 1:
                                    self.stopbits = serial.STOPBITS_ONE
                                case 2:
                                    self.stopbits = serial.STOPBITS_TWO
                                case _:
                                    raise ValueError("Invalid stopbits value")
                            # print debug info
                            if self.debug:
                                print("Driver model: " + self.driver)
                                print("Baudrate: " + str(self.baudrate))
                                print("Bits: " + str(self.bits))
                                print("Parity: " + str(self.parity))
                                print("Stopbits: " + str(self.stopbits))
                            s = serial.Serial(port, self.baudrate, timeout=1, stopbits=self.stopbits, parity=self.parity, bytesize=self.bits)
                            sleep(0.5)
                            # s.write(b"testtesttesttest\n\r")
                            magic = int(self.conf[self.driver]["protocol"]["connection"]["magic"][2::], 16)
                            bytes_val = magic.to_bytes(2, byteorder='big') # convert the magic number to bytes
                            s.write(bytes_val)
                            sleep(0.1)
                            res = s.read(10)
                            sleep(0.1)
                            if self.debug:
                                print("Received: " + str(res))
                            s.close()
                        else: 
                            print("Not implemented")
                    case _:
                        print("Unknown protocol")
            except:
                print(f"Could not connect to board on port: {port}")

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