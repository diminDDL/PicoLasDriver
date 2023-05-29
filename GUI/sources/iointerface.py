import sys
import glob
import serial
import traceback
from time import sleep
from sources.gui import DriverSettings


class IOInterface:
    def __init__(self, debug: bool, config: dict, driver: str, platform: str, driverSettings: DriverSettings):
        # if config is empty throw an error
        if not config:
            raise ValueError("Config is empty")
        self.port = None # the serial port
        self.conf = config
        self.debug = debug
        self.driverSettings = driverSettings
        if self.debug:
            print("IOInterface debug mode enabled")
        self.driver = driver
        self.platform = platform
        if self.conf[self.driver]["protocol"]["connection"]["type"] == "serial":
            self.baudrate = int(self.conf[self.driver]["protocol"]["connection"]["baudrate"])
            self.bits = int(self.conf[self.driver]["protocol"]["connection"]["bits"])
            self.parity = self.conf[self.driver]["protocol"]["connection"]["parity"]
            self.stopbits = int(self.conf[self.driver]["protocol"]["connection"]["stopbits"])

        # self.loop = loop
        # self.loop.run_in_executor(None, self.init_comms)
        self.init_comms()

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
                        # not implemented
                        print("Not implemented")
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
                            self.port = serial.Serial(port, self.baudrate, timeout=3, stopbits=self.stopbits, parity=self.parity, bytesize=self.bits)
                            sleep(1)
                            # s.write(b"testtesttesttest\n\r")
                            magicStr = self.conf[self.driver]["protocol"]["connection"]["magicStr"]
                            # convert the magic string to bytes
                            magic = bytes(magicStr, 'ascii')
                            print("magic: " + str(magic))
                            self.port.write(magic)
                            sleep(0.1)
                            res = self.port.read(len(magic))
                            sleep(0.1)
                            if self.debug:
                                print("Received: " + str(res))
                            # if the result is equal to magic reversed then we break the loop
                            if res == magic[::-1]:
                                break
                            self.port.close()

                        else: 
                            print("Not implemented")
                    case _:
                        print("Unknown protocol")
            except:
                traceback.print_exc()
                # if it's a keyerror, then tell the user that the config file is missing a value
                if sys.exc_info()[0] == KeyError:
                    print("Missing magicStr in config file")
                else:
                    print(f"Could not connect to board on port: {port}")
        if self.port is None:
            print("Could not connect to board")
        else:
            if(self.debug):
                print("Connected to board on port: " + str(self.port.name))
                # TODO proper setup of the board
                self.port.write(bytes("anmo 1", 'ascii'))
                # read out the config
            # start the event loop
            # self.loop.create_task(self.run())
    # at the end of this we need to start some poolling function to keep the event loop running

    def sendAll(self, driverSettings: DriverSettings):
        # send all the settings to the board
        if self.debug:
            print("Sending all settings")
        # send the settings to the board
        if self.port is not None:
            # # send the settings to the board
            # currentCommand = ""
            # currentResponse = ""
            # currentStatus = ""
            # # setCurrent
            # currentCommand = self.driverSettings.setCurrentCommand + " " + str(driverSettings.setCurrent) + "\r\n"
            # self.port.write(bytes(currentCommand, 'ascii'))
            # currentResponse = self.port.read_until(b"\r\n").decode('ascii')
            # currentStatus = self.port.read_until(b"\r\n").decode('ascii')
            # if self.debug:
            #     print("Sent: " + currentCommand)
            #     print("currentResponse: " + currentResponse)
            #     print("currentStatus: " + currentStatus)

            commands = self.driverSettings.getAllCommands()
            print("commands: " + str(commands))
            for command in commands:
                # convert the command to bytes
                # send the command to the board
                # get a reponse and a status
                # write the reponse values into the driverSettings object
                self.port.write(bytes(command, 'ascii'))
                print("Sent: " + command)
                response = self.port.read_until(b"\r\n").decode('ascii')
                status = self.port.read_until(b"\r\n").decode('ascii')
                print("response: " + response)
                print("status: " + status)
                if status.strip() == "00":
                    # get the value from the response
                    value = response.strip()
                    print("command: " + command)
                    print("value: " + value)
                    # write the value to the driverSettings object
                    driverSettings.setValueByCommand(command[:4], value)


            #self.port.write(b"testtesttesttest\n\r")

    def run(self):
        # run the event loop
        while True:
            #print("Running")
            #await asyncio.sleep(1)
            sleep(1)
            #await self.sendAll(self.driverSettings)
            self.sendAll(self.driverSettings)

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