# import serial
# import struct
# import time

# # Header: 8-byte ID, 4-byte file size, 4-byte reserved.
# HEADER_FORMAT = "8sI4s"
# HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
# EOF_MARKER = b"<EOF>\n"

# def send_file(ser, filename, file_id):
#     # Read file data as ASCII text.
#     with open(filename, "r") as f:
#         file_data = f.read()
#     file_bytes = file_data.encode('ascii')
#     file_size = len(file_bytes)
    
#     # Prepare header: pad file_id to 8 bytes.
#     header = struct.pack(HEADER_FORMAT, file_id.encode('ascii').ljust(8, b'_'),
#                          file_size, b'\x00'*4)
    
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
    
#     # Add an inter-file delay so that the receiver can finish processing.
#     time.sleep(1.5)

# def main():
#     # Change port as needed (e.g., '/dev/ttyUSB0' on Linux)
#     ser = serial.Serial("/dev/ttyUSB0", baudrate=115200, timeout=1)
#     time.sleep(2)  # Allow the port to initialize

#     # Send files in order.
#     send_file(ser, "one_sample.dat", "DATAIN")
#     send_file(ser, "w_in.dat", "WIN____")
#     send_file(ser, "w_x.dat", "WX_____")
    
#     ser.close()

# if __name__ == "__main__":
#     main()

import serial
import struct
import time

# Header: 8-byte ID, 4-byte file size, 4-byte reserved.
HEADER_FORMAT = "8sI4s"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
EOF_MARKER = b"<EOF>\n"

def send_file(ser, filename, file_id):
    # Read file data as ASCII text.
    with open(filename, "r") as f:
        file_data = f.read()
    file_bytes = file_data.encode('ascii')
    file_size = len(file_bytes)
    
    # Prepare header: pad file_id to 8 bytes.
    header = struct.pack(HEADER_FORMAT, file_id.encode('ascii').ljust(8, b'_'),
                         file_size, b'\x00'*4)
    
    # Send header.
    ser.write(header)
    ser.flush()
    time.sleep(0.1)
    
    # Send file data.
    ser.write(file_bytes)
    ser.flush()
    time.sleep(0.1)
    
    # Append EOF marker.
    ser.write(EOF_MARKER)
    ser.flush()
    print(f"Sent {filename} with ID {file_id}, size {file_size} bytes.")
    
    # Inter-file delay to ensure the board is ready for the next file.
    time.sleep(1.5)

def main():
    # Change port as needed (e.g., '/dev/ttyUSB0' on Linux)
    ser = serial.Serial("/dev/ttyUSB0", baudrate=115200, timeout=1)
    time.sleep(2)  # Allow the port to initialize

    # Send files in order.
    send_file(ser, "one_sample.dat", "DATAIN")
    send_file(ser, "w_in.dat", "WIN____")
    send_file(ser, "w_x.dat", "WX_____")
    send_file(ser, "w_out.dat", "WOUT____")
    
    ser.close()

if __name__ == "__main__":
    main()
