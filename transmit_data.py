#!/usr/bin/env python3
import socket
import struct
import time
import os

# Get the directory of this script
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Set FILE_PATH to "data/training_data" relative to the script's directory
FILE_PATH = os.path.join(SCRIPT_DIR, "data", "training_data")

print("Using FILE_PATH:", FILE_PATH)

HEADER_FORMAT = "8sI4s"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
EOF_MARKER = b"<EOF>\n"
NUM_INPUTS = 128 # change this if needed

def send_file_tcp(ip, port, filename, file_id):
    """Send a file over TCP with a header and EOF marker."""
    # Construct the full path to the file.
    full_path = os.path.join(FILE_PATH, filename)
    
    # Attempt to open file
    while True:
        try:
            with open(full_path, "r") as f:
                file_data = f.read()
            break
        except Exception as e:
            print(f"Error reading '{full_path}': {e}")
            full_path = input("Please enter a valid full path filename: ").strip()

    file_bytes = file_data.encode('ascii')
    file_size = len(file_bytes)

    # Prepare the header
    header = struct.pack(HEADER_FORMAT,
                         file_id.encode('ascii').ljust(8, b'_'), 
                         file_size,
                         b'\x00' * 4)

    # Create a TCP socket, connect to the board
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        print(f"Connecting to {ip}:{port}...")
        s.connect((ip, port))
        print("Connected. Sending header, file data, and EOF marker.")
        s.sendall(header)
        time.sleep(0.1)
        s.sendall(file_bytes)
        time.sleep(0.1)
        s.sendall(EOF_MARKER)
        time.sleep(0.1)

    print(f"Sent '{filename}' with ID '{file_id}', size {file_size} bytes.\n")

def send_chunk(ip, port, chunk_data, file_id):
    """Send a chunk of data (chunk_data is a string) as a DATAIN file over TCP."""
    file_bytes = chunk_data.encode('ascii')
    file_size = len(file_bytes)
    header = struct.pack(HEADER_FORMAT, file_id.encode('ascii').ljust(8, b'_'),
                         file_size, b'\x00' * 4)
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        print(f"Connecting to {ip}:{port} to send a data chunk...")
        s.connect((ip, port))
        s.sendall(header)
        time.sleep(0.05)
        s.sendall(file_bytes)
        time.sleep(0.05)
        s.sendall(EOF_MARKER)
        time.sleep(0.05)
    print(f"Sent data chunk of size {file_size} bytes.")

def send_data_in_file_in_chunks(ip, port, filename, samples_per_chunk=10):
    """Reads a large DATAIN file and sends it in chunks.
       Each chunk consists of (samples_per_chunk * NUM_INPUTS) floats.
       Assumes one float per line.
    """
    full_path = os.path.join(FILE_PATH, filename)
    with open(full_path, "r") as f:
        lines = f.readlines()
    
    chunk_size = samples_per_chunk * NUM_INPUTS
    
    total_lines = len(lines)
    num_chunks = (total_lines + chunk_size - 1) // chunk_size
    
    print(f"File contains {total_lines} floats; sending in {num_chunks} chunk(s) of {samples_per_chunk} samples each.")
    
    for i in range(num_chunks):
        start = i * chunk_size
        end = min(start + chunk_size, total_lines)
        chunk_data = "".join(lines[start:end])
        send_chunk(ip, port, chunk_data, "DATAIN__")
        time.sleep(0.5)

def send_command(ip, port, cmd):
    """Sends a command over TCP."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((ip, port))
        s.sendall(cmd.encode('ascii'))
        time.sleep(0.1)
    print(f"Sent command: {cmd}")

def main():
    board_ip = "192.168.1.10"  # IP for board (host)
    file_port = 5001           # TCP port on the ZC702
    cmd_port = 5002            # Port for command interactions

    while True:
        print("\nMain Menu:")
        print("m - Send matrix file(s)")
        print("d - Send golden data_out file")
        print("t - Toggle training (on/off)")
        print("e - Run ESN (select data_in)")
        print("r - Soft reset board (all or just data)")
        print("q - Quit")

        choice = input("Enter your choice: ").strip().lower()
        if choice == 'm':
            print("\nMatrix file options:")
            print("a - Send w_in.dat")
            print("b - Send w_x.dat")
            print("c - Send w_out.dat")
            print("d - Send w_in and w_x (if training w_out)")
            print("e - Send all three matrix files (w_in, w_x, w_out)")
            matrix_choice = input("Enter your option (a/b/c/d/e): ").strip().lower()

            if matrix_choice == 'a':
                send_file_tcp(board_ip, file_port, "w_in.dat", "WIN_____")
            elif matrix_choice == 'b':
                send_file_tcp(board_ip, file_port, "w_x.dat", "WX______")
            elif matrix_choice == 'c':
                send_file_tcp(board_ip, file_port, "w_out.dat", "WOUT____")
            elif matrix_choice == 'd':
                send_file_tcp(board_ip, file_port, "w_in.dat", "WIN_____")
                send_file_tcp(board_ip, file_port, "w_x.dat", "WX______")
            elif matrix_choice == 'e':
                send_file_tcp(board_ip, file_port, "w_in.dat", "WIN_____")
                send_file_tcp(board_ip, file_port, "w_x.dat", "WX______")
                send_file_tcp(board_ip, file_port, "w_out.dat", "WOUT____")
            else:
                print("Invalid matrix file option. Please try again.")

        elif choice == 'd':
            data_out_filename = input("Enter the DATAOUT filename to send (e.g., data_out.dat): ").strip()
            while not os.path.isfile(os.path.join(FILE_PATH, data_out_filename)):
                print(f"File '{data_out_filename}' not found in {FILE_PATH}.")
                data_out_filename = input("Please enter a valid DATAOUT filename: ").strip()
            send_file_tcp(board_ip, file_port, data_out_filename, "DATAOUT_")

        elif choice == 't':
            print("\nTraining options:")
            print("1 - Turn training OFF")
            print("2 - Turn training ON")
            reset_choice = input("Enter your option (1/2): ").strip().lower()

            if reset_choice == '1':
                send_command(board_ip, cmd_port, "TRN_OFF")
            elif reset_choice == '2':
                send_command(board_ip, cmd_port, "TRN_ON")

        elif choice == 'e':
            print("\nESN options:")
            print("1 - Send entire data_in")
            print("2 - Send chunks of larger data_in")
            esn_choice = input("Enter your option (1/2): ").strip().lower()

            if esn_choice == '1':
                data_filename = input("Enter the DATAIN filename to send (e.g., one_sample.dat): ").strip()
                while not os.path.isfile(os.path.join(FILE_PATH, data_filename)):
                    print(f"File '{data_filename}' not found in {FILE_PATH}.")
                    data_filename = input("Please enter a valid DATAIN filename: ").strip()
                send_file_tcp(board_ip, file_port, data_filename, "DATAIN___")
            elif esn_choice == '2':
                data_filename = input("Enter the DATAIN filename to send (e.g., data_in.dat): ").strip()
                while not os.path.isfile(os.path.join(FILE_PATH, data_filename)):
                    print(f"File '{data_filename}' not found in {FILE_PATH}.")
                    data_filename = input("Please enter a valid DATAIN filename: ").strip()
                send_data_in_file_in_chunks(board_ip, file_port, data_filename, samples_per_chunk=10)

        elif choice == 'r':
            print("\nReset options:")
            print("1 - Reset everything")
            print("2 - Reset just data in")
            reset_choice = input("Enter your option (1/2): ").strip().lower()

            if reset_choice == '1':
                send_command(board_ip, cmd_port, "RESET")
            elif reset_choice == '2':
                send_command(board_ip, cmd_port, "RDI")

        elif choice == 'q':
            print("Exiting.")
            break
        else:
            print("Invalid option. Try again.")

if __name__ == "__main__":
    main()