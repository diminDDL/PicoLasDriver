import sys
import json
import sources.gui as gui
import sources.iointerface as ioint
import asyncio

debug = False
version = "0.0.1"
#driverModel = "LDP-CW 20-50"
driverModel = "LDP-C 40-05"
platform = "Arduiono DUE"

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
        -m "driver model" | --model "driver model": provide the model of the driver to use (must be in the config.json file)
        -f | --fullscreen: run the program in fullscreen mode without any way of escaping it
        -p "platform" | --platform "platform": provide the platform of the driver to use (must be in the config.json file)""")
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

if "-p" in sys.argv or "--platform" in sys.argv:
    try:
        platform = sys.argv[sys.argv.index("-p") + 1]
    except IndexError:
        print("Error: no platform provided")
        sys.exit(1)
    except ValueError:
        try:
            platform = sys.argv[sys.argv.index("--platform") + 1]
        except IndexError:
            print("Error: no platform provided")
            sys.exit(1)
    platform.strip("\"")
    print("Using platform: " + platform)

if "-f" in sys.argv or "--fullscreen" in sys.argv:
    fullscreen = True
else:
    fullscreen = False

# run the program if this is this file being executed
if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    driverSettings = gui.DriverSettings()
    io = ioint.IOInterface(debug, config, driverModel, platform, loop)
    window = gui.GUI(loop=loop, version=version, config=config, driver=driverModel, platform=platform, io=io, debug=debug, fullscreen=fullscreen, driverSettings=driverSettings)
    #loop.run_forever()
    loop.run_until_complete(window.updater(window.interval))
    loop.close()
    exit(0)
    