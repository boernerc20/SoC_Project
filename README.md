# SoC_Project
- Use simpleUART Vivado project (sets up functionality for two UARTs: one for debugging, one for transmission/recieption of files) for hardware and bitstream (use two_uart.bit)
- SoC_Project houses the src files for Vitis and building C code on the FPGA processing system.
- Data folder has data_in, W_x, W_in, state_tosave, W_out, and data_out .dat files that the C code uses
- transmit_data.py (in data folder) is used to transmit the three input files to the board over UART0 for processing
- Open a serial terminal on UART1 to monitor any debugging statements
