import sys
import json
import sources.gui as gui
import asyncio

debug = False
version = "0.0.1"
driverModel = "LDP-CW 20-50"

# open the config.json file and read the settings
f = open("config.json", "r")
config = json.load(f)
f.close()

# argument parsing
if "-d" in sys.argv or "--debug" in sys.argv:
    debug = True
if "-h" in sys.argv or "--help" in sys.argv:
    print("""Usage: run.py [options]
        Use ctrl+slash to leave the fullscrren view. In normal mode, use escape in debug mode.
        Description:
        -d | --debug: run in debug mode (Esc button to exit the fullscreen view)
        -h | --help: show this help message and exit
        -c | --console: run a simple serial console to configure the driver
        -v | --version: show the version of the program and exit
        -m "driver model" | --model "driver model": provide the model of the driver to use (must be in the config.json file)""")
    sys.exit(0)
if "-c" in sys.argv or "--console" in sys.argv:
    print("console mode not implemented yet")
    sys.exit(0)

if "-v" in sys.argv or "--version" in sys.argv:
    print("PicoLas controller version " + version)
    sys.exit(0)

if "-m" in sys.argv or "--model" in sys.argv:
    try:
        driverModel = sys.argv[sys.argv.index("-m") + 1]
    except IndexError:
        print("Error: no model provided")
        sys.exit(1)
    except ValueError:
        try:
            driverModel = sys.argv[sys.argv.index("--model") + 1]
        except IndexError:
            print("Error: no model provided")
            sys.exit(1)
    driverModel.strip("\"")
    print("Using driver model: " + driverModel)

# run the program if this is this file being executed
if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    window = gui.GUI(loop=loop, version=version, config=config, driver=driverModel, debug=debug)
    loop.run_forever()
    loop.close()
    