# SoC_Project
1.) Pull GitHub repo into its own directory (away from Vitis or Vivado project files)\
2.) Import Vitis ZC702_File project and make sure to use base_ps_wrapper.xsa for hardware\
4.) When setting up run configuration make sure base_ps_wrapper.bit is used to program the board\
5.) Open a UART port on debugger USB-UART bridge of board\
5.) Build and run Vitis project on board, should see a startup message on debugger port\
6.) Assign a static ethernet connection on the same subnet as the board, ping to test connection\
7.) To send files run the transmit_data.py script, will use port 5001 (within the data/ directory)\
8.) File transmission can be seen in debugger port with final y_out generated as well\
