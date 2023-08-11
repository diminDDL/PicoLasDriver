## Step-by-Step Instructions for Setting Up the Raspberry Pi for the Picolas Controller

1. **Setup Your RPi GUI**:
   - Boot your Raspberry Pi and enter the desktop environment.
   
2. **Enable 'No Login Shell'**:
   - Access `raspi-config`:
     ```bash
     sudo raspi-config
     ```
   - Navigate to and enable 'No Login Shell'.
   
3. **Disable Auto Sleep (Optional)**:
   - Modify power-saving settings based on your OS and desktop environment.

4. **Install TK-Dev**:
   ```bash
   sudo apt install tk-dev
   ```

5. **Update Python to 3.10 with TK**:
   - Follow guidance specific to updating Python on Raspberry Pi ensuring TK support.

6. **Change Serial Port to PL011**:
   - Refer to the solution provided in this [StackOverflow post](https://stackoverflow.com/questions/65900026/pyserial-readline-is-blocking-although-timeout-is-defined).

7. **Disable Serial in raspi-config**:
   - Re-access `raspi-config` and navigate to disable the serial port.

8. **Add a Startup Script**:
   - Create a script to automatically run the provided Python program on startup:
     ```bash
     python /path/to/file/run.py -f
     ```

9. **Manage Touchscreen Functionality**:
   - Use the provided script to disable the touchscreen as the X11 session starts.
   - Utilize another script to re-enable the touchscreen from within the Python program.
   
