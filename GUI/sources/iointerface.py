import sys
import glob
import serial
import asyncio
import serial_asyncio
import traceback
from time import sleep
import datetime
from sources.gui import DriverSettings

# TODO setup of the driver

class SerialDriver(asyncio.Protocol):
    def __init__(self, driverSettings: DriverSettings, debug: bool = False, port: str = '/dev/ttyUSB0', baudrate: int = 115200, bits: int = 8, parity: str = 'N', stopbits: int = 1):
        self.driverSettings = driverSettings
        self.old_driverSettings = driverSettings.copy()
        self.debug = debug
        self.port = port
        self.baudrate = baudrate
        self.bits = bits
        self.parity = parity
        self.stopbits = stopbits
        self.enabled = False
        self.sending = False
        self.first_start = True
        self.buffer = b""
        self.command_queue = asyncio.Queue()
        self.is_sending = asyncio.Lock()
        self.command_processed = asyncio.Event()  # Flag to indicate a command has been processed
        self.response_timeout = 1.0  # Timeout value in seconds

    def connection_made(self, transport):
        self.transport = transport
        self.connected = True
        if self.debug:
            print("Connection made")

    def data_received(self, data):
        self.buffer += data

    async def wait_for_complete_message(self):
        """Wait for a complete response of the form '{value}\r\n{status}\r\n'."""
        while self.buffer.count(b'\r\n') < 2:
            await asyncio.sleep(0.1)
        end_of_message = self.buffer.index(b'\r\n', self.buffer.index(b'\r\n') + 2)
        complete_message = self.buffer[:end_of_message]
        if b'\r\n' in complete_message:  # Check if the message has the correct format
            return complete_message
        else:
            return None

    async def process_buffer(self):
        while self.connected:
            try:
                message = await asyncio.wait_for(self.wait_for_complete_message(), timeout=0.1)
                self.process_data(message + b'\r\n')
                self.buffer = self.buffer[len(message) + 2:]
            except asyncio.TimeoutError:
                if self.buffer:
                    self.process_data(self.buffer)
                    self.buffer = b""

    def process_data(self, data):
        if self.debug:
            print("Data received: ", data)
        split_data = data.split(b"\r\n")
        if len(split_data) >= 2:
            response, status = split_data[:2]
            response = response.decode('ascii')
            status = status.decode('ascii')
            if self.debug:
                print("response: ", response)
                print("status: ", status)
            if status.strip() == "00":
                value = response.strip()
                if self.debug:
                    print("value: ", value)
                if value:  # Only set the value if it's not empty
                    try:
                        self.driverSettings.setValueByCommand(self.current_command[:4], value)
                    except ValueError as e:
                        print(f"Error setting value for command {self.current_command[:4]}: {e}")
                    finally:
                        self.command_processed.set()  # Always set the event, even if an error occurs

        else:
            if self.debug:
                print("Received unexpected data format.")

    async def sendAll(self, mode : int = 0):
        self.sending = True
        if mode == 0:
            commands = self.driverSettings.getAllCommands()
        elif mode == 1:
            commands = self.driverSettings.getChangedCommands(self.old_driverSettings)
            self.old_driverSettings = self.driverSettings.copy()
        elif mode == 2:
            commands = self.driverSettings.getReadCommands()
        for command in commands:
            await self.command_queue.put(command)
        self.enabled = False

        # Wait for all commands to be processed before setting sending to False
        await self.command_queue.join()
        self.sending = False

    async def wait_for_command_processed(self):
        while not self.command_processed.is_set():
            await asyncio.sleep(0.1)
        self.command_processed.clear()  # Reset the flag

    async def process_queue(self):
        while True:
            command = await self.command_queue.get()

            async with self.is_sending:
                self.current_command = command
                self.transport.write(bytes(command, 'ascii'))
                if self.debug:
                    print("Sent: ", command)
                
                try:
                    await asyncio.wait_for(self.wait_for_command_processed(), timeout=self.response_timeout)
                except asyncio.TimeoutError:
                    print(f"Warning: Response to command {command} timed out.")
                    
            self.command_queue.task_done()
            if self.debug:
                print("Done sending command: ", command)

    async def main(self, loop):
        self.transport, _ = await serial_asyncio.create_serial_connection(loop, lambda: self, self.port, baudrate=self.baudrate, bytesize=self.bits, parity=self.parity, stopbits=self.stopbits)

        last_time = loop.time()

        asyncio.ensure_future(self.process_buffer())
        asyncio.ensure_future(self.process_queue())

        while True:
            if self.enabled:
                if self.first_start:
                    await self.sendAll(mode=0)
                    self.first_start = False
                else:
                    await self.sendAll(mode=1)
            elif loop.time() - last_time >= 1 and not self.sending:     #TODO decrease this value
                last_time = loop.time()
                await self.sendAll(mode=2)

            await asyncio.sleep(0.1)



class IOInterface:
    def __init__(self, debug: bool, config: dict, driver: str, platform: str, driverSettings: DriverSettings, loop: asyncio.AbstractEventLoop):
        # if config is empty throw an error
        if not config:
            raise ValueError("Config is empty")
        self.port = None # the serial port
        self.conf = config
        self.debug = debug
        self.driverSettings = driverSettings
        self.correctDevice = False
        if self.debug:
            print("IOInterface debug mode enabled")
        self.driver = driver
        self.platform = platform
        if self.conf[self.driver]["protocol"]["connection"]["type"] == "serial":
            self.baudrate = int(self.conf[self.driver]["protocol"]["connection"]["baudrate"])
            self.bits = int(self.conf[self.driver]["protocol"]["connection"]["bits"])
            self.parity = self.conf[self.driver]["protocol"]["connection"]["parity"]
            self.stopbits = int(self.conf[self.driver]["protocol"]["connection"]["stopbits"])

        self.maxCurent = self.conf[self.driver]['MaxCurrent_A']
        self.minPuseWidth = self.conf[self.driver]['MinPulseWidth_us']/1000000
        self.maxPulseWidth = self.conf[self.driver]['MaxPulseWidth_us']/1000000
        self.maxFrequency = self.conf[self.driver]['MaxFrequency_Hz']

        self.loop = loop
        self.serialDriver = None
        # self.loop.run_in_executor(None, self.init_comms)
        # TODO this blocks the event loop, figure out how to make it async
        self.init_comms()
        self.serialDriver = SerialDriver(self.driverSettings, self.debug, self.port.name, self.baudrate, self.bits, self.parity, self.stopbits)
        self.serialDriver.enabled = self.correctDevice

    def convert_to_type(self, data_type, value):
        try:
            if data_type == "boolean":
                if value == "0":
                    return False
                elif value == "1":
                    return True
                else:
                    return None
            elif data_type == "64bit":
                data_type = "int"
            elif data_type == "32bit":
                data_type = "int"
            converted_value = eval(f"{data_type}({value})")
            return converted_value
        except (SyntaxError, NameError, TypeError, ValueError):
            return None

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
                            self.port = serial.Serial(port, self.baudrate, timeout=5.0, stopbits=self.stopbits, parity=self.parity, bytesize=self.bits)
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
                                self.correctDevice = True

                                if(self.debug):
                                    print("Connected to board on port: " + str(self.port.name))
                                # setup of the board
                                # read out the config

                                # set the maximum current
                                self.port.reset_input_buffer()
                                sleep(0.1)
                                command = self.conf[self.driver]["protocol"]["io_commands"]["set_max_current"]["command"]
                                msg_type = self.conf[self.driver]["protocol"]["io_commands"]["set_max_current"]["parameter"]
                                message = bytes(command + " " + str(self.maxCurent) +"\r\n", 'ascii')
                                print("Sending: " + str(message))
                                self.port.write(message)
                                sleep(0.1)
                                # keep reading until we get a \r\n
                                res = self.port.read_until(b"\r\n").decode('ascii').strip("\r\n")
                                status = self.port.read_until(b"\r\n")
                                # convert the result to the correct type
                                res = self.convert_to_type(msg_type, res)
                                expected_res = self.convert_to_type(msg_type, self.maxCurent)
                                if res == expected_res:
                                    pass
                                else:
                                    print("Error setting maximum current")
                                
                                if self.debug:
                                    print("Expected: " + str(expected_res))
                                    print("Received: " + str(res))
                                    print("Status: " + str(status))
                                sleep(0.1)

                                # read in the previous set current if it's above the maximum current then set it to the maximum current, store in config variable
                                self.port.reset_input_buffer()
                                self.port.reset_output_buffer()
                                sleep(0.1)
                                command = self.conf[self.driver]["protocol"]["get_current"]["command"]
                                msg_type = self.conf[self.driver]["protocol"]["get_current"]["answer"]
                                message = bytes(command + "\r\n", 'ascii')
                                print("Sending: " + str(message))
                                self.port.write(message)
                                sleep(0.1)
                                res = self.port.read_until(b"\r\n").decode('ascii').strip("\r\n")
                                status = self.port.read_until(b"\r\n")
                                res = self.convert_to_type(msg_type, res)
                                if res > self.maxCurent:
                                    res = self.maxCurent
                                    # send the current to the board
                                    self.port.reset_input_buffer()
                                    self.port.reset_output_buffer()
                                    sleep(0.1)
                                    command = self.conf[self.driver]["protocol"]["set_current"]["command"]
                                    msg_type = self.conf[self.driver]["protocol"]["set_current"]["parameter"]
                                    message = bytes(command + " " + str(self.driverSettings.setCurrent) +"\r\n", 'ascii')
                                    print("Sending: " + str(message))
                                    self.port.write(message)
                                    sleep(0.1)
                                    # keep reading until we get a \r\n
                                    res = self.port.read_until(b"\r\n").decode('ascii').strip("\r\n")
                                    status = self.port.read_until(b"\r\n")
                                    # convert the result to the correct type
                                    res = self.convert_to_type(msg_type, res)
                                    expected_res = self.convert_to_type(msg_type, self.maxCurent)
                                    if res == expected_res:
                                        pass
                                    else:
                                        print("Error setting current")

                                    sleep(0.1)

                                    if self.debug:
                                        print("Expected: " + str(expected_res))
                                        print("Received: " + str(res))
                                        print("Status: " + str(status))
                                self.driverSettings.setCurrent = res
                                sleep(0.1)

                                if self.debug:
                                    print("Received: " + str(res))
                                    print("Status: " + str(status))

                                # read frequency, store in config variable
                                for comm in ["get_pulse_duration", "get_pulse_frequency"]:
                                    msg_type = self.conf[self.driver]["protocol"]["io_commands"][comm]["answer"]
                                    command = self.conf[self.driver]["protocol"]["io_commands"][comm]["command"]
                                    message = bytes(command + "\r\n", 'ascii')
                                    print("Sending: " + str(message))
                                    self.port.write(message)
                                    sleep(0.1)
                                    res = self.port.read_until(b"\r\n").decode('ascii').strip("\r\n")
                                    status = self.port.read_until(b"\r\n")
                                    res = self.convert_to_type(msg_type, res)
                                    if comm == "get_pulse_duration":
                                        if self.minPuseWidth <= res <= self.maxPulseWidth:
                                            self.driverSettings.setPulseWidth = res/1000
                                    else:
                                        if 0 <= res <= self.maxFrequency:
                                            self.driverSettings.setFrequency = res

                                    if self.debug:
                                        print("Received: " + str(res))
                                        print("Status: " + str(status))

                                # set mode to analog
                                self.port.reset_input_buffer()
                                self.port.reset_output_buffer()
                                command = self.conf[self.driver]["protocol"]["io_commands"]["set_analog_mode"]["command"]
                                msg_type = self.conf[self.driver]["protocol"]["io_commands"]["set_analog_mode"]["parameter"]
                                message = bytes(command + " 1\r\n", 'ascii')
                                print("Sending: " + str(message))
                                self.port.write(message)
                                sleep(0.1)
                                res = self.port.read_until(b"\r\n").decode('ascii').strip("\r\n")
                                status = self.port.read_until(b"\r\n")
                                res = self.convert_to_type(msg_type, res)
                                if res != True:
                                    print("Error setting mode to analog")

                                if self.debug:
                                    print("Received: " + str(res))
                                    print("Status: " + str(status))

                                self.port.close()
                                break
                            else:
                                self.correctDevice = False
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
        if not self.correctDevice:
            print("Could not connect to board")
            self.port = None
            # throw an error
        else:
            pass

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