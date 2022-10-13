import serial
import json
from time import sleep


class IOInterface:
    def __init__(self, debug: bool, config: dict):
        # if config is empty throw an error
        if not config:
            raise ValueError("Config is empty")
        self.conf = config
        self.debug = debug

    def execute(self, command: str, args):
        # execute a command
        if self.debug:
            print("Executing command: " + command)