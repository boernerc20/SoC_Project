

#!/usr/bin/env python3
import socket
import struct
import time
import os

HEADER_FORMAT = "8sI4s"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)
EOF_MARKER = b"<EOF>\n"

def send_file_tcp(ip, port, filename, file_id):
    """Send a file over TCP with a header and EOF marker."""
    # Attempt to open file
    while True:
        try:
            with open(filename, "r") as f:
                file_data = f.read()
            break
        except Exception as e:
            print(f"Error reading '{filename}': {e}")
            filename = input("Please enter a valid filename: ").strip()

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

        # Send header
        s.sendall(header)
        time.sleep(0.1)
        # Send file content
        s.sendall(file_bytes)
        time.sleep(0.1)
        # Send EOF marker
        s.sendall(EOF_MARKER)
        time.sleep(0.1)

    print(f"Sent '{filename}' with ID '{file_id}', size {file_size} bytes.\n")


def main():
    board_ip = "192.168.1.10"  # IP for board (host)
    board_port = 5001          # TCP port on the ZC702

    while True:
        print("\nMain Menu:")
        print("1 - Send matrix file(s)")
        print("2 - Send data file")
        print("3 - Perform ESN calculation on board")
        print("r - Soft reset board")
        print("q - Quit")

        choice = input("Enter your choice: ").strip().lower()
        if choice == '1':
            print("\nMatrix file options:")
            print("a - Send w_in.dat")
            print("b - Send w_x.dat")
            print("c - Send w_out.dat")
            print("d - Send all three matrix files (w_in, w_x, w_out)")
            matrix_choice = input("Enter your option (a/b/c/d): ").strip().lower()

            if matrix_choice == 'a':
                send_file_tcp(board_ip, board_port, "w_in.dat",  "WIN_____")
            elif matrix_choice == 'b':
                send_file_tcp(board_ip, board_port, "w_x.dat",   "WX______")
            elif matrix_choice == 'c':
                send_file_tcp(board_ip, board_port, "w_out.dat", "WOUT____")
            elif matrix_choice == 'd':
                send_file_tcp(board_ip, board_port, "w_in.dat",  "WIN_____")
                send_file_tcp(board_ip, board_port, "w_x.dat",   "WX______")
                send_file_tcp(board_ip, board_port, "w_out.dat", "WOUT____")
            else:
                print("Invalid matrix file option. Please try again.")

        elif choice == '2':
            data_filename = input("Enter the DATAIN filename to send (e.g., one_sample.dat): ").strip()
            while not os.path.isfile(data_filename):
                print(f"File '{data_filename}' not found.")
                data_filename = input("Please enter a valid DATAIN filename: ").strip()
            send_file_tcp(board_ip, board_port, data_filename, "DATAIN___")
        
        elif choice == '3':
            # Send a tiny "command" file to the board with file_id = "CMD_ESN_"
            send_file_tcp(board_ip, board_port, "cmd_esn.txt", "CMD_ESN_")

        elif choice == 'r':
            print("\nReset options:")
            print("1 - Reset everything")
            print("2 - Reset just data in")
            reset_choice = input("Enter your option (1/2): ").strip().lower()

            if reset_choice == '1':
                send_file_tcp(board_ip, board_port, "cmd_rst.txt", "CMD_RST_")
            elif reset_choice == '2':
                send_file_tcp(board_ip, board_port, "cmd_rdi.txt", "CMD_RDI_")

        elif choice == 'q':
            print("Exiting.")
            break
        else:
            print("Invalid option. Try again.")

if __name__ == "__main__":
    main()