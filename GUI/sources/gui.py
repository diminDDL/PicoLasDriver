import asyncio
import tkinter as tk
from random import randrange as rr
import si_prefix as si

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
        self.loop = loop
        self.tasks = []
        self.tasks.append(loop.create_task(self.mainloop()))
        self.tasks.append(loop.create_task(self.updater(interval)))

        # used for development on computer, sets the screen size to 800 x 480 pixels
        if self.debug:
            self.root.geometry("800x480")
            self.root.bind("<Escape>", self.close)
        else:
            self.root.attributes("-fullscreen", True)
            self.root.bind("<Control-slash>", self.close)

        self.root.title("PicoLas controller window")
        self.root.resizable(False, False)
        self.root.attributes("-topmost", True)
        self.createMainWindow(version)
        # self.root.overrideredirect(True) # maybe used when you don't want the toolbar, but we use full screen so it's not needed
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

    def createMainWindow(self, version):
        # to be replaced with the main window
        self.root.title("PicoLas controller window - version " + version)

        self.root.columnconfigure(0, weight=1)
        self.root.columnconfigure(1, weight=1)
        self.root.columnconfigure(2, weight=1)
        self.root.columnconfigure(3, weight=1)

        version_label = tk.Label(self.root, text="V" + version, fg="#bdbdbd")
        version_label.grid(column=0, row=0, sticky=tk.NW)
        version_label = tk.Label(self.root, text="Using driver: " + self.driver, fg="#8f8f8f")
        version_label.grid(column=1, row=0, sticky=tk.NW)
        current_limit_label = tk.Label(self.root, text=f"Current limit\n[0 - {si.si_format(self.maxCurent)}A]")
        current_limit_label.grid(column=0, row=1, sticky=tk.N, padx=5, pady=5)
        pulse_duration_label = tk.Label(self.root, text=f"Pulse duration\n[{si.si_format(self.minPuseWidth)}s - {si.si_format(self.maxPulseWidth)}s]")
        pulse_duration_label.grid(column=0, row=2, sticky=tk.N, padx=5, pady=5)
        pulse_frequency_label = tk.Label(self.root, text=f"Pulse frequency\n[0 Hz - {si.si_format(self.maxFrequency)}Hz]")
        pulse_frequency_label.grid(column=0, row=3, sticky=tk.N, padx=5, pady=5)
        # TODO add photo diode readout and status display in this column
        self.setCurrentSrt = tk.Variable()
        self.setCurrentSrt.set('Value set:\n0.0A')
        current_limit = tk.Label(self.root, textvariable=self.setCurrentSrt, fg="#ffffff", bg="#ff8c00")
        current_limit.grid(column=1, row=1, sticky=tk.N, padx=10, pady=5)
        self.setPulseWidthSrt = tk.Variable()
        self.setPulseWidthSrt.set('Value set:\n0.0s')
        pulse_duration = tk.Label(self.root, textvariable=self.setPulseWidthSrt, fg="#ffffff", bg="#ff8c00")
        pulse_duration.grid(column=1, row=2, sticky=tk.N, padx=10, pady=5)
        self.setFrequencySrt = tk.Variable()
        self.setFrequencySrt.set('Value set:\n0.0Hz')
        pulse_frequency = tk.Label(self.root, textvariable=self.setFrequencySrt, fg="#ffffff", bg="#ff8c00")
        pulse_frequency.grid(column=1, row=3, sticky=tk.N, padx=10, pady=5)
