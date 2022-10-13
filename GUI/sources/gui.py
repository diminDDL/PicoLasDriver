import asyncio
import tkinter as tk
from random import randrange as rr

def deg_color(deg, d_per_tick, color):
    deg += d_per_tick
    if 360 <= deg:
        deg %= 360
        color = '#%02x%02x%02x' % (rr(0, 256), rr(0, 256), rr(0, 256))
    return deg, color

class GUI:
    def __init__(self, loop, interval=1/120, debug=False):
        self.debug = debug
        self.root = tk.Tk()
        self.loop = loop
        self.tasks = []
        self.tasks.append(loop.create_task(self.rotator(1/60, 2)))
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
        # self.root.overrideredirect(True) # maybe used when you don't want the toolbar, but we use fullscreen so it's not needed
        self.root.after(1000, self.stayAlive)
         
    async def rotator(self, interval, d_per_tick):
        canvas = tk.Canvas(self.root, height=600, width=600)
        canvas.pack()
        deg = 0
        color = 'black'
        arc = canvas.create_arc(100, 100, 500, 500, style=tk.CHORD,
                                start=0, extent=deg, fill=color)
        while await asyncio.sleep(interval, True):
            deg, color = deg_color(deg, d_per_tick, color)
            canvas.itemconfigure(arc, extent=deg, fill=color)

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

    async def displayVersion(self, version):
        self.root.title("PicoLas controller window - version " + version)
        # TODO adjust the cords later
        tk.Label(self.root, text="Version: " + version).place(x=0, y=0)

    def stayAlive(self):
        # to be replaced with communication with the laser controller
        # print("stay alive")
        self.root.after(1000, self.stayAlive)

    async def createMainWindow(self):
        # to be replaced with the main window
        self.mainWindow = tk.Frame(self.root)
        tk.Label(self.root, text="Current limit\n[0.1-20A]")