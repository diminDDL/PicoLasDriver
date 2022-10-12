#!/bin/sh

# This script disables the touch screen on the Raspberry Pi 7" Touchscreen, this has to be run in rc.local
# to disable the touch screen before the X server starts.

# list all xinput devices and store them in a variable
xinput_devices=$(xinput --list)
# find the device containing "FT5406" and store it's ID in a variable
touch_device_id=$(echo "$xinput_devices" | grep -i "FT5406" | grep -o -E "id=[0-9]+" | grep -o -E "[0-9]+")
# print the detected device ID
echo "Touch device ID: $touch_device_id"
# store the device id into an environment variable
export TOUCH_DEVICE_ID=$touch_device_id
# disable the touch device
xinput --disable $touch_device_id