import asyncio
import tkinter as tk
from random import randrange as rr
import si_prefix as si
import time

def deg_color(deg, d_per_tick, color):
    deg += d_per_tick
    if 360 <= deg:
        deg %= 360
        color = '#%02x%02x%02x' % (rr(0, 256), rr(0, 256), rr(0, 256))
    return deg, color

class GUI:
    def __init__(self, loop, version, config, driver, interval=1/60, debug=False):
        self.debug = debug
        self.version = version
        self.config = config
        self.driver = driver
        self.maxCurent = self.config[self.driver]['MaxCurrent_A']
        self.minPuseWidth = self.config[self.driver]['MinPulseWidth_us']/1000000
        self.maxPulseWidth = self.config[self.driver]['MaxPulseWidth_us']/1000000
        self.maxFrequency = self.config[self.driver]['MaxFrequency_Hz']
        self.setCurrent = 0.0
        self.setPuseWidth = 0.0
        self.setFrequency = 0.0
        self.root = tk.Tk()
        self.setCurrentSrt = tk.Variable()
        self.setCurrentSrt.set('Value set:\n0.0A')
        self.setPulseWidthSrt = tk.Variable()
        self.setPulseWidthSrt.set('Value set:\n0.0s')
        self.setFrequencySrt = tk.Variable()
        self.setFrequencySrt.set('Value set:\n0.0Hz')
        self.loop = loop
        self.tasks = []
        self.tasks.append(loop.create_task(self.mainloop()))
        self.tasks.append(loop.create_task(self.updater(interval)))

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
        else:
            width= self.root.winfo_screenwidth()               
            height= self.root.winfo_screenheight()               
            self.root.geometry("%dx%d" % (width, height))
            #self.root.attributes("-type", "splash")
            self.root.bind("<Control-slash>", self.close)
            self.root.focus_force()
            self.root.config(cursor="none")
            self.root.overrideredirect(True)

        self.root.title("PicoLas controller window")
        self.root.resizable(False, False)
        self.root.attributes("-topmost", True)
        self.createMainWindow(version)
        self.root.after(1000, self.comm)

    async def mainloop(self):
        pass

    async def updater(self, interval):
        while True:
            self.root.update()
            await asyncio.sleep(interval)

    def close(self, event=None):
        print("closing window")
        for task in self.tasks:
            task.cancel()
        self.loop.stop()
        self.root.destroy()
        
    def comm(self):
        # to be replaced with communication with the laser controller
        # print("stay alive")
        self.root.after(1000, self.comm)

    def updateDisplayValues(self):
        # update the display values
        self.setCurrentSrt.set("Value set:\n" + str(si.si_format(self.setCurrent, precision=1)) + "A")
        self.setPulseWidthSrt.set("Value set:\n" + str(si.si_format(self.setPuseWidth, precision=3)) + "s")
        self.setFrequencySrt.set("Value set:\n" + str(si.si_format(self.setFrequency, precision=0)) + "Hz")

    def adjustValues(self, command=None, pressedTime=0):
        # change values, command provides a string describing what should change
        # print("timeDelta: " + str(pressedTime - self.GUIlastCall) + " acceleration: " + str(self.GUIcallAcceleration) + " callNumber: " + str(self.GUIcallNumber))
        if pressedTime - self.GUIlastCall >= self.GUIcallAcceleration:
            if pressedTime - self.GUIlastCall <= 250:
                self.GUIcallNumber += 1
                self.GUIcallAcceleration -= 2
            else:
                self.GUIcallNumber = 1
                self.GUIcallAcceleration = 100
            self.GUIlastCall = pressedTime
            if command == "currentUp":
                self.setCurrent += 0.1
                if self.setCurrent > self.maxCurent:
                    self.setCurrent = self.maxCurent
                    #TODO implement color changes
                    self.current_limit_up.configure()
            elif command == "currentDown":
                self.setCurrent -= 0.1
                if self.setCurrent < 0:
                    self.setCurrent = 0
            elif command == "pulseWidthUp":
                if self.GUIcallNumber >= 50:
                    self.setPuseWidth += round(self.GUIcallNumber/50000, 3)
                else:
                    self.setPuseWidth += 0.001
                if self.setPuseWidth > self.maxPulseWidth:
                    self.setPuseWidth = self.maxPulseWidth
            elif command == "pulseWidthDown":
                if self.GUIcallNumber >= 50:
                    self.setPuseWidth -= round(self.GUIcallNumber/50000, 3)
                else:
                    self.setPuseWidth -= 0.001
                if self.setPuseWidth < self.minPuseWidth:
                    self.setPuseWidth = self.minPuseWidth
            elif command == "frequencyUp":
                if self.GUIcallNumber >= 50:
                    self.setFrequency += round(self.GUIcallNumber/50, 0)
                else:
                    self.setFrequency += 1
                if self.setFrequency > self.maxFrequency:
                    self.setFrequency = self.maxFrequency
            elif command == "frequencyDown":
                if self.GUIcallNumber >= 50:
                    self.setFrequency -= round(self.GUIcallNumber/50, 0)
                else:
                    self.setFrequency -= 1
                if self.setFrequency < 0:
                    self.setFrequency = 0
        
        self.updateDisplayValues()

    def createMainWindow(self, version):
        # to be replaced with the main window
        self.root.title("PicoLas controller window - version " + version)

        colNum = 5
        rowNum = 8

        for i in range(colNum):
            self.root.columnconfigure(i, weight=1)
        for i in range(rowNum):
            self.root.rowconfigure(i, weight=1)

        

        # general labels
        self.version_label = tk.Label(self.root, text="V" + version, fg="#bdbdbd")
        self.version_label.grid(column=0, row=0, sticky=tk.NW)
        self.version_label = tk.Label(self.root, text="Using driver: " + self.driver, fg="#8f8f8f")
        self.version_label.grid(column=1, row=0, sticky=tk.NW)
        # labels for the variables
        self.current_limit_label = tk.Label(self.root, text=f"Current limit\n[0 - {si.si_format(self.maxCurent)}A]", font=("Arial", 15))
        self.current_limit_label.grid(column=0, row=1, sticky="nsew", padx=5, pady=5)
        self.pulse_duration_label = tk.Label(self.root, text=f"Pulse duration\n[{si.si_format(self.minPuseWidth)}s - {si.si_format(self.maxPulseWidth)}s]", font=("Arial", 15))
        self.pulse_duration_label.grid(column=0, row=2, sticky="nsew", padx=5, pady=5)
        self.pulse_frequency_label = tk.Label(self.root, text=f"Pulse frequency\n[0 Hz - {si.si_format(self.maxFrequency)}Hz]", font=("Arial", 15))
        self.pulse_frequency_label.grid(column=0, row=3, sticky="nsew", padx=5, pady=5)
        # TODO add photo diode readout and status display in this column
        # value displays

        self.current_limit = tk.Label(self.root, textvariable=self.setCurrentSrt, fg="#ffffff", bg="#ff8c00", font=("Arial", 15))
        self.current_limit.grid(column=1, row=1, sticky="nsew", padx=10, pady=5)

        self.pulse_duration = tk.Label(self.root, textvariable=self.setPulseWidthSrt, fg="#ffffff", bg="#ff8c00", font=("Arial", 15))
        self.pulse_duration.grid(column=1, row=2, sticky="nsew", padx=10, pady=5)

        self.pulse_frequency = tk.Label(self.root, textvariable=self.setFrequencySrt, fg="#ffffff", bg="#ff8c00", font=("Arial", 15))
        self.pulse_frequency.grid(column=1, row=3, sticky="nsew", padx=10, pady=5)
        # buttons to change the values
        self.current_limit_up = tk.Button(self.root, text="+", command= lambda: self.adjustValues("currentUp", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("Arial", 20, "bold"), fg="#ffffff", bg="#00dd00", activebackground="#00ff00", activeforeground="#ffffff")
        self.current_limit_up.grid(column=2, row=1, sticky="nsew", padx=5, pady=5)
        self.current_limit_down = tk.Button(self.root, text="-", command= lambda: self.adjustValues("currentDown", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("Arial", 20, "bold"), fg="#ffffff", bg="#dd0000", activebackground="#ff0000", activeforeground="#ffffff")
        self.current_limit_down.grid(column=3, row=1, sticky="nsew", padx=5, pady=5)
        self.pulse_duration_up = tk.Button(self.root, text="+", command= lambda: self.adjustValues("pulseWidthUp", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("Arial", 20, "bold"), fg="#ffffff", bg="#00dd00", activebackground="#00ff00", activeforeground="#ffffff")
        self.pulse_duration_up.grid(column=2, row=2, sticky="nsew", padx=5, pady=5)
        self.pulse_duration_down = tk.Button(self.root, text="-", command= lambda: self.adjustValues("pulseWidthDown", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("Arial", 20, "bold"), fg="#ffffff", bg="#dd0000", activebackground="#ff0000", activeforeground="#ffffff")
        self.pulse_duration_down.grid(column=3, row=2, sticky="nsew", padx=5, pady=5)
        self.pulse_frequency_up = tk.Button(self.root, text="+", command= lambda: self.adjustValues("frequencyUp", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("Arial", 20, "bold"), fg="#ffffff", bg="#00dd00", activebackground="#00ff00", activeforeground="#ffffff")
        self.pulse_frequency_up.grid(column=2, row=3, sticky="nsew", padx=5, pady=5)
        self.pulse_frequency_down = tk.Button(self.root, text="-", command= lambda: self.adjustValues("frequencyDown", pressedTime=round(time.time() * 1000)), repeatinterval=10, repeatdelay=300, font=("Arial", 20, "bold"), fg="#ffffff", bg="#dd0000", activebackground="#ff0000", activeforeground="#ffffff")
        self.pulse_frequency_down.grid(column=3, row=3, sticky="nsew", padx=5, pady=5)
