# import serial
# import struct
# import time

# # Constants for header and EOF marker
# HEADER_FORMAT = "8sI4s"
# HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
# EOF_MARKER = b"<EOF>\n"

# def send_file(ser, filename, file_id):
#     # Try to open the file. If it doesn't exist, keep prompting the user.
#     file_data = None
#     while file_data is None:
#         try:
#             with open(filename, "r") as f:
#                 file_data = f.read()
#         except FileNotFoundError:
#             print(f"File '{filename}' not found. Please enter a valid filename.")
#             filename = input("Enter the data_in filename (e.g., one_sample.dat): ").strip()

#     file_bytes = file_data.encode('ascii')
#     file_size = len(file_bytes)
    
#     # Prepare header: pad file_id to 8 bytes.
#     header = struct.pack(HEADER_FORMAT,
#                          file_id.encode('ascii').ljust(8, b'_'),
#                          file_size,
#                          b'\x00' * 4)
    
#     # Send header.
#     ser.write(header)
#     ser.flush()
#     time.sleep(0.1)
    
#     # Send file data.
#     ser.write(file_bytes)
#     ser.flush()
#     time.sleep(0.1)
    
#     # Append EOF marker.
#     ser.write(EOF_MARKER)
#     ser.flush()
#     print(f"Sent {filename} with ID {file_id}, size {file_size} bytes.")
    
#     # Inter-file delay to ensure the board is ready for the next file.
#     time.sleep(1.5)

# def main():
#     # Change port as needed (for example, '/dev/ttyUSB0' on Linux)
#     ser = serial.Serial("/dev/ttyUSB0", baudrate=115200, timeout=1)
#     time.sleep(2)  # Allow the port to initialize

#     # Send matrix files when the user is ready.
#     input("Press Enter to send matrix files (w_in.dat, w_x.dat, w_out.dat)...")
#     send_file(ser, "w_in.dat", "WIN____")
#     send_file(ser, "w_x.dat", "WX_____")
#     send_file(ser, "w_out.dat", "WOUT____")
#     print("Matrix files sent.")

#     # Wait for the user to trigger sending the DATAIN file.
#     data_filename = input("Enter the data_in filename to send (e.g., one_sample.dat): ").strip()
#     send_file(ser, data_filename, "DATAIN")
    
#     ser.close()
#     print("Transmission complete.")

# if __name__ == "__main__":
#     main()
#!/usr/bin/env python3
import serial
import struct
import time
import sys
import os

# Constants for header and EOF marker.
HEADER_FORMAT = "8sI4s"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
EOF_MARKER = b"<EOF>\n"

def send_file(ser, filename, file_id):
    """Send a file over serial with header and EOF marker.
       If the file does not exist, prompt until it does."""
    file_data = None
    while file_data is None:
        try:
            with open(filename, "r") as f:
                file_data = f.read()
        except Exception as e:
            print(f"Error reading file '{filename}': {e}")
            filename = input("Please enter a valid filename: ").strip()
    file_bytes = file_data.encode('ascii')
    file_size = len(file_bytes)
    
    header = struct.pack(HEADER_FORMAT,
                         file_id.encode('ascii').ljust(8, b'_'),
                         file_size,
                         b'\x00' * 4)
    
    ser.write(header)
    ser.flush()
    time.sleep(0.1)
    ser.write(file_bytes)
    ser.flush()
    time.sleep(0.1)
    ser.write(EOF_MARKER)
    ser.flush()
    print(f"Sent '{filename}' with ID {file_id}, size {file_size} bytes.")
    time.sleep(1.5)
    return True

def main():
    try:
        ser = serial.Serial("/dev/ttyUSB0", baudrate=115200, timeout=1)
    except Exception as e:
        print(f"Error opening serial port: {e}")
        sys.exit(1)
    time.sleep(2)  # Allow the port to initialize

    print("Interactive Transmission Mode")
    print("===================================")
    while True:
        print("\nMenu:")
        print("1 - Send matrix file")
        print("2 - Send DATAIN file")
        print("q - Quit")
        choice = input("Enter your choice: ").strip()
        if choice == '1':
            print("\nMatrix file options:")
            print("a - Send w_in.dat")
            print("b - Send w_x.dat")
            print("c - Send w_out.dat")
            print("d - Send all matrix files")
            matrix_choice = input("Enter your option (a/b/c/d): ").strip().lower()
            if matrix_choice == 'a':
                send_file(ser, "w_in.dat", "WIN_____")
            elif matrix_choice == 'b':
                send_file(ser, "w_x.dat", "WX______")
            elif matrix_choice == 'c':
                send_file(ser, "w_out.dat", "WOUT____")
            elif matrix_choice == 'd':
                send_file(ser, "w_in.dat", "WIN_____")
                send_file(ser, "w_x.dat", "WX______")
                send_file(ser, "w_out.dat", "WOUT____")
            else:
                print("Invalid option. Please try again.")
        elif choice == '2':
            data_filename = input("Enter the DATAIN filename to send (e.g., one_sample.dat): ").strip()
            while not os.path.isfile(data_filename):
                print(f"File '{data_filename}' not found.")
                data_filename = input("Please enter a valid DATAIN filename: ").strip()
            send_file(ser, data_filename, "DATAIN")
        elif choice.lower() == 'q':
            print("Quitting transmission mode.")
            break
        else:
            print("Invalid option. Please try again.")
    ser.close()
    print("Transmission complete.")

if __name__ == "__main__":
    main()
