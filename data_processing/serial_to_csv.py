#!/usr/bin/env python3
import serial
import time

# Open the serial port. Adjust the device name as needed.
# For example, if your ZedBoard is connected via a USB-to-UART adapter that appears as /dev/ttyUSB0:
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

# Path to output file
output_file = "data_output.csv"

print("Listening on /dev/ttyUSB0 at 115200 baud...")

while True:
    try:
        # Read a full line (until newline is received) from the serial port
        line = ser.readline().decode('ascii', errors='replace').strip()
        
        # If a non-empty line was received:
        if line:
            print("Received:", line)
            # Open the CSV output file in write mode (this clears any previous data)
            with open(output_file, "w") as f:
                f.write(line + "\n")
            print("Data written to", output_file)
            
        # A short sleep to prevent busy waiting
        time.sleep(0.1)
        
    except KeyboardInterrupt:
        print("Exiting...")
        break
    except Exception as e:
        print("Error:", e)
        break

ser.close()
