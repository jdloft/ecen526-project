import serial
import re
import sys

# Check if correct number of command-line arguments are provided
if len(sys.argv) != 3:
    print("Usage: python script.py <serial_port> <file_name>")
    sys.exit(1)

# Get serial port and file name from command-line arguments
serial_port = sys.argv[1]
file_name = sys.argv[2]

# Open serial port
ser = serial.Serial(serial_port, 115200)  # Adjust the baud rate as needed

# Open file for writing binary data
file = open(file_name, 'w')

# Regular expression to match timestamp and formatting
escape_reg = re.compile(r'\[0(;[0-9]*)?m')

try:
    while True:
        # Read a line from serial port
        try:
            line = ser.readline().decode().strip()  # Assuming lines are terminated with newline character
        except Exception:
            continue

        # Print to console for display
        print(line)

        line = escape_reg.sub('', line)
        file.write(line + '\n')
        file.flush()  # Ensure data is written immediately

except KeyboardInterrupt:
    # Close serial port and file on keyboard interrupt
    ser.close()
    file.close()
    print("Exiting...")
