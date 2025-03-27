# SoC_Project
1.) Pull GitHub repo into its own directory (away from Vitis/Vivado project files)
2.) Create blank Vitis project using two_uart.xsa hardware file (in /simple_uart/)
3.) Copy files (main.c, platfrom.c, platform.h) from /SoC_Project/src into new project src
4.) When setting up run configuration make sure two_uart.bit is used to program the board
5.) Open a UART port on debugger USB-UART bridge of board (should be UART0)
5.) Build and run Vitis project on board, should see a startup message on debugger port
6.) To send files run the transmit_data.py script (within the /data/ directory)
7.) File transmission can be seen in debugger port with final y_out generated as well
