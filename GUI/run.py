import sys
import json
import sources.gui as gui
import asyncio

debug = False
version = "0.0.1"

# open the config.json file and read the settings
f = open("config.json", "r")
config = json.load(f)
f.close()

# argument parsing
if "-d" in sys.argv or "--debug" in sys.argv:
    debug = True
if "-h" in sys.argv or "--help" in sys.argv:
    print("""Usage: run.py [-d | --debug] [-h | --help]
        Use ctrl+slash to leave the fullscrren view.
        Description:
        -d | --debug: run in debug mode (Esc button to exit the fullscreen view)
        -h | --help: show this help message and exit
        -c | --console: run a simple serial console to configure the driver""")
    sys.exit(0)
if "-c" in sys.argv or "--console" in sys.argv:
    print("console mode not implemented yet")
    sys.exit(0)

# run the program if this is this file being executed
if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    window = gui.GUI(loop, debug=debug)
    loop.run_until_complete(window.rotator(1/60, 2))
    loop.run_forever()
    loop.close()
    # window.displayVersion(version)
    # window.root.mainloop()
    