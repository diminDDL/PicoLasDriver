import asyncio
import tkinter as tk
from random import randrange as rr
from dataclasses import dataclass
import si_prefix as si
import time
import os

# TODO add data send on no change logic

def deg_color(deg, d_per_tick, color):
    deg += d_per_tick
    if 360 <= deg:
        deg %= 360
        color = '#%02x%02x%02x' % (rr(0, 256), rr(0, 256), rr(0, 256))
    return deg, color


class DriverSettings:
    def __init__(self):
        self.setCurrent = 0.0
        self.setPulseWidth = 0.0
        self.setFrequency = 0.0
        self.setPulseMode = False
        self.globalPulseCounter = 0
        self.localPulseCounter = 0
        self.ADCReadoutValue = 0
        self.gpio_0 = False
        self.gpio_1 = False
        self.gpio_2 = False
        self.gpio_3 = False
        self.globalEnable = False

    def setCommands(self, setCurrent, setPulseWidth, setFrequency, setPulseMode, globalPulseCounter, localPulseCounter, ADCReadoutValue, gpio, globalEnable):
        self.setCurrentCommand = setCurrent
        self.setPulseWidthCommand = setPulseWidth
        self.setFrequencyCommand = setFrequency
        self.setPulseModeCommand = setPulseMode
        self.globalPulseCounterCommand = globalPulseCounter
        self.localPulseCounterCommand = localPulseCounter
        self.ADCReadoutValueCommand = ADCReadoutValue
        self.gpioCommand = gpio
        self.globalEnableCommand = globalEnable

    def __eq__(self, other):
        if not isinstance(other, DriverSettings):
            return NotImplemented
        return self.__dict__ == other.__dict__
    
    def copy(self):
        new_instance = DriverSettings()
        new_instance.setCurrent = self.setCurrent
        new_instance.setPulseWidth = self.setPulseWidth
        new_instance.setFrequency = self.setFrequency
        new_instance.setPulseMode = self.setPulseMode
        new_instance.globalPulseCounter = self.globalPulseCounter
        new_instance.localPulseCounter = self.localPulseCounter
        new_instance.ADCReadoutValue = self.ADCReadoutValue
        new_instance.gpio_0 = self.gpio_0
        new_instance.gpio_1 = self.gpio_1
        new_instance.gpio_2 = self.gpio_2
        new_instance.gpio_3 = self.gpio_3
        new_instance.globalEnable = self.globalEnable
        return new_instance
    
    def getAllCommands(self):
        command_attrs = [attr for attr in self.__dict__ if 'Command' in attr]
        commands_list = []
        for attr in command_attrs:
            if attr == "gpioCommand":
                # Collect the gpio attributes and convert them to 8 bit binary string
                gpio_attrs = [f"gpio_{i}" for i in range(4)]
                gpio_values = [int(getattr(self, gpio)) for gpio in gpio_attrs]
                binary_gpio_values = "".join(map(str, gpio_values))
                padded_binary_gpio_values = binary_gpio_values.rjust(8, '0')
                decimal_gpio_values = int(padded_binary_gpio_values, 2)

                commands_list += [f"{getattr(self, attr)} " + str(decimal_gpio_values) + "\r\n"]
            elif attr == "setPulseWidthCommand":
                commands_list += [f"{getattr(self, attr)} " + str(int(self.setPulseWidth * 1000000)) + "\r\n"]
            else:
                value_attr = attr.replace("Command", "")
                value = getattr(self, value_attr)
                curr_atr = getattr(self, attr, None)
                if isinstance(value, bool):
                    value = int(value)
                elif value is None or value == "":
                    continue
                if curr_atr is None:
                    continue
                if str(curr_atr).startswith("g"):   # if it starts with a g it means it's a get command
                    commands_list += [f"{curr_atr}" + "\r\n"]
                else:
                    commands_list += [f"{curr_atr} " + str(value) + "\r\n"]
        
        return commands_list

    def setValueByCommand(self, command, value):
        floatAttrs = ["setCurrent", "setPulseWidth", "setFrequency"]
        intAttrs = ["globalPulseCounter", "localPulseCounter", "ADCReadoutValue"]
        boolAttrs = ["setPulseMode", "gpio_0", "gpio_1", "gpio_2", "gpio_3", "globalEnable"]

        command_attrs = [attr for attr in dir(self) if 'Command' in attr]
        for attr in command_attrs:
            if getattr(self, attr) == command:
                value_attr = attr.replace("Command", "")
                print(f"Found command '{command}' for attribute '{value_attr}'")
                if value_attr == "gpio":  # Handle GPIO command as a special case
                    # we need to unpack the int into 4 bools
                    binary_gpio_values = bin(int(value))[2:].zfill(4)
                    print("binary_gpio_values: " + binary_gpio_values)
                    for i, bit in enumerate(binary_gpio_values):
                        setattr(self, f"gpio_{i}", bool(int(bit)))
                    print(f"Set GPIOs to {binary_gpio_values}")
                    return
                elif hasattr(self, value_attr):
                    if value_attr in floatAttrs:
                        if value_attr == "setPulseWidth":
                            value = float(value)/1000000
                        setattr(self, value_attr, float(value))
                    elif value_attr in intAttrs:
                        setattr(self, value_attr, int(value))
                    elif value_attr in boolAttrs:
                        setattr(self, value_attr, bool(int(value)))
                    else:
                        setattr(self, value_attr, value)
                    print(f"Set {value_attr} to {value}")
                    return
        print(f"Command '{command}' not found")


class GUI:
    def __init__(self, loop, version, config, driver, platform, io, driverSettings: DriverSettings, interval=1/60, debug=False, fullscreen=False):
        self.debug = debug
        if self.debug:
            print("GUI debug mode enabled")
        self.version = version
        self.config = config
        self.driver = driver
        self.platform = platform
        self.io = io
        self.interval = interval
        self.maxCurent = self.config[self.driver]['MaxCurrent_A']
        self.minPuseWidth = self.config[self.driver]['MinPulseWidth_us']/1000000
        self.maxPulseWidth = self.config[self.driver]['MaxPulseWidth_us']/1000000
        self.maxFrequency = self.config[self.driver]['MaxFrequency_Hz']

        self.driv = driverSettings
        self.driv.setPulseWidth = self.minPuseWidth
        self.old_driv = DriverSettings()

        self.driv.setCommands(setCurrent=self.config[self.driver]["protocol"]["set_current"]["command"],
                                setPulseWidth=self.config[self.driver]["protocol"]["io_commands"]["set_pulse_duration"]["command"],
                                setFrequency=self.config[self.driver]["protocol"]["io_commands"]["set_pulse_frequency"]["command"],
                                setPulseMode=None,
                                globalPulseCounter=self.config[self.driver]["protocol"]["io_commands"]["get_global_pulse_count"]["command"],
                                localPulseCounter=self.config[self.driver]["protocol"]["io_commands"]["get_current_pulse_count"]["command"],
                                ADCReadoutValue=self.config[self.driver]["protocol"]["io_commands"]["get_adc"]["command"],
                                gpio=self.config[self.driver]["protocol"]["io_commands"]["set_gpio"]["command"],
                                globalEnable=self.config[self.driver]["protocol"]["io_commands"]["output_enable"]["command"]
                              )

        self.root = tk.Tk()
        self.setCurrentSrt = tk.Variable()
        self.setPulseWidthSrt = tk.Variable()
        self.setFrequencySrt = tk.Variable()
        self.setPulseModeSrt = tk.Variable()
        self.ADCReadoutStr = tk.Variable()
        self.ADCReadoutStr.set("Photodiode:\n 0")
        self.globalPulseCounterLabel = tk.Variable()
        self.globalPulseCounterLabel.set('Global pulse\n counter:\n' + '0')
        self.localPulseCounterLabel = tk.Variable()
        self.localPulseCounterLabel.set('Pulse counter:\n' + '0')
        self.errorReadout = "No Errors"
        self.errorReadoutOld = ""
        self.driv.globalEnable = False

        # read the ./sources/assets/ folder and copy the images into a dictionary with the file name as key
        self.images = {}
        for file in os.listdir("./sources/assets/"):
            if file.endswith(".png"):
                self.images[file[:-4]] = tk.PhotoImage(file="./sources/assets/" + file).subsample(3, 3)
        self.locked = False

        self.loop = loop
        self.tasks = []

        self.GUIlastCall = 0
        self.GUIcallNumber = 1
        self.GUIcallAcceleration = 100

        # used for development on computer, sets the screen size to 800 x 480 pixels
        if self.debug:
            self.root.geometry("800x480")
            self.Xres = 800
            self.Yres = 480
            self.root.bind("<Escape>", self.close)
            self.root.focus_force()
            #bind the close button to the close function
            self.root.protocol("WM_DELETE_WINDOW", self.close)
        else:
            width= self.root.winfo_screenwidth()               
            height= self.root.winfo_screenheight()               
            self.root.geometry("%dx%d" % (width, height))
            if fullscreen:
                self.root.attributes("-type", "splash")
            self.root.bind("<Control-slash>", self.close)
            self.root.focus_force()
            self.root.config(cursor="none")
            if fullscreen:
                self.root.overrideredirect(True)

        self.root.title("PicoLas controller window")
        self.root.resizable(False, False)
        self.root.attributes("-topmost", True)
        self.createMainWindow(version)
        self.loop.create_task(self.comm())

    async def updater(self, interval):
        while True:
            self.root.update()
            await asyncio.sleep(interval)

    def close(self, event=None):
        print("closing window")
        self.root.destroy()
        self.loop.stop()
        
    async def comm(self):
        while True:
            # communication with the laser controller will go here
            await asyncio.sleep(0.5)
            
            if self.debug:
                print("checking for changes in driv")
            # check if old driv is different from new driv
            if self.old_driv != self.driv:
                if self.debug:
                    print("driv changed")
                # if driv changed, send the new values to the controller
                self.old_driv = self.driv.copy()
                # self.io.send(self.driv)


            # update the UI and restart the keep alive function
            self.updateDisplayValues()

    def adjustValues(self, command=None, pressedTime=0):
        # change values, command provides a string describing what should change
        callNumberThreshold = 50
        # print("timeDelta: " + str(pressedTime - self.GUIlastCall) + " acceleration: " + str(self.GUIcallAcceleration) + " callNumber: " + str(self.GUIcallNumber))
        if pressedTime - self.GUIlastCall >= self.GUIcallAcceleration:
            if pressedTime - self.GUIlastCall <= 250:
                self.GUIcallNumber += 1
                self.GUIcallAcceleration -= 2
            else:
                self.GUIcallNumber = 1
                self.GUIcallAcceleration = 100
            self.GUIlastCall = pressedTime
            if command == "currentUp" and not self.locked:
                self.driv.setCurrent += self.config[self.driver]['CurrentStep_A']
                if self.driv.setCurrent > self.maxCurent:
                    self.driv.setCurrent = self.maxCurent
                    #TODO implement color changes
                    #self.current_limit_up.configure()
            elif command == "currentDown" and not self.locked:
                self.driv.setCurrent -= self.config[self.driver]['CurrentStep_A']
                if self.driv.setCurrent < self.config[self.driver]['CurrentStep_A']:
                    self.driv.setCurrent = 0
            elif command == "pulseWidthUp" and not self.locked:
                if self.GUIcallNumber >= callNumberThreshold:
                    self.driv.setPulseWidth += round(self.GUIcallNumber/(callNumberThreshold * 1000), 3)
                else:
                    self.driv.setPulseWidth += 0.001
                if self.driv.setPulseWidth > self.maxPulseWidth:
                    self.driv.setPulseWidth = self.maxPulseWidth
            elif command == "pulseWidthDown" and not self.locked:
                if self.GUIcallNumber >= callNumberThreshold:
                    self.driv.setPulseWidth -= round(self.GUIcallNumber/(callNumberThreshold * 1000), 3)
                else:
                    self.driv.setPulseWidth -= 0.001
                if self.driv.setPulseWidth < self.minPuseWidth:
                    self.driv.setPulseWidth = self.minPuseWidth
            elif command == "frequencyUp" and not self.locked:
                if self.GUIcallNumber >= callNumberThreshold:
                    self.driv.setFrequency += round(self.GUIcallNumber/callNumberThreshold, 0)
                else:
                    self.driv.setFrequency += 1
                if self.driv.setFrequency > self.maxFrequency:
                    self.driv.setFrequency = self.maxFrequency
            elif command == "frequencyDown" and not self.locked:
                if self.GUIcallNumber >= callNumberThreshold:
                    self.driv.setFrequency -= round(self.GUIcallNumber/callNumberThreshold, 0)
                else:
                    self.driv.setFrequency -= 1
                if self.driv.setFrequency < 0:
                    self.driv.setFrequency = 0
            elif command == "lock":
                if self.GUIcallNumber == 6:
                    self.locked = not self.locked
            elif command == "enable" and not self.locked:
                self.driv.globalEnable = not self.driv.globalEnable
            
        
        self.updateDisplayValues()

    def togglePulseMode(self):
        if not self.locked:
            self.driv.setPulseMode = not self.driv.setPulseMode
        self.updateDisplayValues()

    def gpioButton(self, gpio):
        if not self.locked:
            if gpio == 0:
                self.driv.gpio_0 = not self.driv.gpio_0
            elif gpio == 1:
                self.driv.gpio_1 = not self.driv.gpio_1
            elif gpio == 2:
                self.driv.gpio_2 = not self.driv.gpio_2
            elif gpio == 3:
                self.driv.gpio_3 = not self.driv.gpio_3
        self.updateDisplayValues()

    def createMainWindow(self, version):
        # to be replaced with the main window
        self.root.title("PicoLas controller window - version " + version)

        colNum = 6
        rowNum = 8

        for i in range(colNum):
            self.root.columnconfigure(i, weight=1)
        for i in range(rowNum):
            self.root.rowconfigure(i, weight=1)

        # general labels
        self.version_label = tk.Label(self.root, text="V" + version, fg="#bdbdbd", font=("Arial", 8))
        self.version_label.grid(column=0, row=0, sticky=tk.NW)
        self.version_label = tk.Label(self.root, text="Using driver: " + self.driver, fg="#8f8f8f", font=("Arial", 8))
        self.version_label.grid(column=2, row=0, sticky=tk.N)
        
        # labels for the variables
        self.current_limit_label = tk.Label(self.root, text=f"Current limit\n[0 - {si.si_format(self.maxCurent)}A]", font=("Arial", 15))
        self.current_limit_label.grid(column=0, row=1, sticky="nsew", padx=5, pady=5)
        self.pulse_duration_label = tk.Label(self.root, text=f"Pulse duration\n[{si.si_format(self.minPuseWidth)}s - {si.si_format(self.maxPulseWidth)}s]", font=("Arial", 15))
        self.pulse_duration_label.grid(column=0, row=2, sticky="nsew", padx=5, pady=5)
        self.pulse_frequency_label = tk.Label(self.root, text=f"Pulse frequency\n[0 Hz - {si.si_format(self.maxFrequency)}Hz]", font=("Arial", 15))
        self.pulse_frequency_label.grid(column=0, row=3, sticky="nsew", padx=5, pady=5)
        
        # ADC readout
        self.adc_label = tk.Label(self.root, textvariable=self.ADCReadoutStr, font=("Arial", 15), fg="white", bg="black")
        self.adc_label.grid(column=0, row=4, sticky="nsew", padx=5, pady=5)

        # Error readout
        self.error_text = tk.Text(self.root, height=3, width=20, font=("Arial", 15), fg="white", bg="black", wrap=tk.WORD, state=tk.DISABLED)
        self.error_text.grid(column=0, row=5, sticky="nsew", padx=5, pady=5)
        # create a scroll bar and associate it with the error text
        self.error_scrollbar = tk.Scrollbar(self.root, command=self.error_text.yview)
        self.error_scrollbar.grid(column=1, row=5, sticky="wns", pady=5)
        self.error_text['yscrollcommand'] = self.error_scrollbar.set

        # value displays
        self.current_limit = tk.Label(self.root, textvariable=self.setCurrentSrt, fg="#ffffff", bg="#ff8c00", font=("Arial", 15))
        self.current_limit.grid(column=1, columnspan=2, row=1, sticky="nsew", padx=10, pady=5)

        self.pulse_duration = tk.Label(self.root, textvariable=self.setPulseWidthSrt, fg="#ffffff", bg="#ff8c00", font=("Arial", 15))
        self.pulse_duration.grid(column=1, columnspan=2, row=2, sticky="nsew", padx=10, pady=5)

        self.pulse_frequency = tk.Label(self.root, textvariable=self.setFrequencySrt, fg="#ffffff", bg="#ff8c00", font=("Arial", 15))
        self.pulse_frequency.grid(column=1, columnspan=2, row=3, sticky="nsew", padx=10, pady=5)
        
        # 4 GPIO buttons
        self.gpioFrame = tk.Frame(self.root)
        self.gpioFrame.columnconfigure(4, weight=1)
        self.gpioFrame.rowconfigure(1, weight=1)
        self.gpioFrame.grid(column=2, columnspan=4, row=4, sticky="nsew", padx=5, pady=5)

        self.gpio_button_0 = tk.Button(self.gpioFrame, text="\nIO1\n", command=lambda: self.gpioButton(0), font=("Arial", 12), fg="#ffffff", bg="black", activebackground="black", activeforeground="#ffffff")
        self.gpio_button_0.grid(column=0, row=0, padx=(15, 5), pady=5)

        self.gpio_button_1 = tk.Button(self.gpioFrame, text="\nIO2\n", command=lambda: self.gpioButton(1), font=("Arial", 12), fg="#ffffff", bg="black", activebackground="black", activeforeground="#ffffff")
        self.gpio_button_1.grid(column=1, row=0, padx=5, pady=5)

        self.gpio_button_2 = tk.Button(self.gpioFrame, text="\nIO3\n", command=lambda: self.gpioButton(2), font=("Arial", 12), fg="#ffffff", bg="black", activebackground="black", activeforeground="#ffffff")
        self.gpio_button_2.grid(column=2, row=0, padx=5, pady=5)

        self.gpio_button_3 = tk.Button(self.gpioFrame, text="\nIO4\n", command=lambda: self.gpioButton(3), font=("Arial", 12), fg="#ffffff", bg="black", activebackground="black", activeforeground="#ffffff")
        self.gpio_button_3.grid(column=3, row=0, padx=5, pady=5)

        # buttons to change the values
        self.current_limit_up = tk.Button(self.root, text="+", width=2, command= lambda: self.adjustValues("currentUp", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("TkFixedFont", 20, "bold"), fg="#ffffff", bg="#00dd00", activebackground="#00dd00", activeforeground="#ffffff")
        self.current_limit_up.grid(column=3, row=1, sticky="nsew", padx=5, pady=5)

        self.current_limit_down = tk.Button(self.root, text="-", width=2, command= lambda: self.adjustValues("currentDown", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("TkFixedFont", 20, "bold"), fg="#ffffff", bg="#dd0000", activebackground="#dd0000", activeforeground="#ffffff")
        self.current_limit_down.grid(column=4, row=1, sticky="nsew", padx=5, pady=5)

        self.pulse_duration_up = tk.Button(self.root, text="+", width=2, command= lambda: self.adjustValues("pulseWidthUp", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("TkFixedFont", 20, "bold"), fg="#ffffff", bg="#00dd00", activebackground="#00dd00", activeforeground="#ffffff")
        self.pulse_duration_up.grid(column=3, row=2, sticky="nsew", padx=5, pady=5)

        self.pulse_duration_down = tk.Button(self.root, text="-", width=2, command= lambda: self.adjustValues("pulseWidthDown", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("TkFixedFont", 20, "bold"), fg="#ffffff", bg="#dd0000", activebackground="#dd0000", activeforeground="#ffffff")
        self.pulse_duration_down.grid(column=4, row=2, sticky="nsew", padx=5, pady=5)

        self.pulse_frequency_up = tk.Button(self.root, text="+", width=2, command= lambda: self.adjustValues("frequencyUp", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("TkFixedFont", 20, "bold"), fg="#ffffff", bg="#00dd00", activebackground="#00dd00", activeforeground="#ffffff")
        self.pulse_frequency_up.grid(column=3, row=3, sticky="nsew", padx=5, pady=5)

        self.pulse_frequency_down = tk.Button(self.root, text="-", width=2, command= lambda: self.adjustValues("frequencyDown", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("TkFixedFont", 20, "bold"), fg="#ffffff", bg="#dd0000", activebackground="#dd0000", activeforeground="#ffffff")
        self.pulse_frequency_down.grid(column=4, row=3, sticky="nsew", padx=5, pady=5)

        # pulse mode button
        self.pulse_mode = tk.Button(self.root, textvariable=self.setPulseModeSrt, command=lambda: self.togglePulseMode(), font=("Arial", 15), fg="#ffffff", bg="black", activebackground="black", activeforeground="#ffffff")
        self.pulse_mode.grid(column=5, row=3, sticky="nsew", padx=5, pady=5)

        # pulse counter indicators
        self.pulse_number_label = tk.Label(self.root, textvariable=self.globalPulseCounterLabel, fg="#bdbdbd", bg="black")
        self.pulse_number_label.grid(column=5, row=1, sticky="nsew", padx=9, pady=5)

        self.pulse_number = tk.Label(self.root, textvariable=self.localPulseCounterLabel, fg="#bdbdbd", bg="black")
        self.pulse_number.grid(column=5, row=2, sticky="nsew", padx=9, pady=5)
        
        # lock button to lock the values
        self.lock_button = tk.Button(self.root, image=self.images["unlocked"], command=lambda: self.adjustValues("lock", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, fg="#ffffff", bg="green", activebackground="green", activeforeground="#ffffff")
        self.lock_button.grid(column=5, row=4, sticky="nsew", padx=5, pady=5)

        # global enable button
        self.enable_button = tk.Button(self.root, text="DISABLED", command=lambda: self.adjustValues("enable", pressedTime=round(time.time() * 1000)), fg="#ffffff", bg="red", activebackground="red", activeforeground="#ffffff")
        self.enable_button.grid(column=2, columnspan=4, row=5, sticky="nsew", padx=5, pady=5)

        self.updateDisplayValues()

        
    def updateDisplayValues(self):
        # update the display values
        self.setCurrentSrt.set("Value set:\n" + str(str(round(self.driv.setCurrent, 1)) if self.driv.setCurrent >= self.config[self.driver]['CurrentStep_A'] else '0.0') + "A")
        
        # cap the pulse width depending on the frequency
        if self.driv.setFrequency > 0:
            if self.driv.setPulseWidth > 1/self.driv.setFrequency:
                self.driv.setPulseWidth = 1/self.driv.setFrequency
        else:
            self.driv.setPulseWidth = 0

        if self.driv.setPulseWidth <= 1: 
            self.setPulseWidthSrt.set("Value set:\n" + str(si.si_format(self.driv.setPulseWidth, precision=0)) + "s")
        else:
            self.setPulseWidthSrt.set("Value set:\n" + str(si.si_format(self.driv.setPulseWidth, precision=3)) + "s")
        self.setFrequencySrt.set("Value set:\n" + str(si.si_format(self.driv.setFrequency, precision=0)) + "Hz")

        # update the globalPulseCounterLabel
        self.globalPulseCounterLabel.set("Global pulse counter:\n" + str(self.driv.globalPulseCounter))

        # update the localPulseCounterLabel
        self.localPulseCounterLabel.set("Local pulse counter:\n" + str(self.driv.localPulseCounter))

        # update adc readout
        self.ADCReadoutStr.set("ADC readout:\n" + str(self.driv.ADCReadoutValue))

        # update the pulse mode
        self.setPulseModeSrt.set(f"Pulse mode:\n{'Pulsed' if self.driv.setPulseMode else 'Single'}")
        self.pulse_mode.configure(textvariable=self.setPulseModeSrt, bg='green' if self.driv.setPulseMode else 'black', activebackground='green' if self.driv.setPulseMode else 'black')
        # update the GPIO buttons
        self.gpio_button_0.configure(bg='green' if self.driv.gpio_0 else 'black', activebackground='green' if self.driv.gpio_0 else 'black')
        self.gpio_button_1.configure(bg='green' if self.driv.gpio_1 else 'black', activebackground='green' if self.driv.gpio_1 else 'black')
        self.gpio_button_2.configure(bg='green' if self.driv.gpio_2 else 'black', activebackground='green' if self.driv.gpio_2 else 'black')
        self.gpio_button_3.configure(bg='green' if self.driv.gpio_3 else 'black', activebackground='green' if self.driv.gpio_3 else 'black')
        
        # if the self.errorReadout changed update the error label
        if self.errorReadout != self.errorReadoutOld:
            self.errorReadoutOld = self.errorReadout
            self.error_text.configure(state=tk.NORMAL)
            self.error_text.delete(1.0, tk.END)
            self.error_text.insert(tk.END, self.errorReadout)
            self.error_text.configure(state=tk.DISABLED)
        
        # update the locked button image
        self.lock_button.configure(image=self.images["locked" if self.locked else "unlocked"], activebackground="red" if self.locked else "green", background="red" if self.locked else "green")

        # update the enable button
        self.enable_button.configure(text="ENABLED" if self.driv.globalEnable else "DISABLED", bg="green" if self.driv.globalEnable else "red", activebackground="green" if self.driv.globalEnable else "red")