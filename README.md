# SoC Project

This project creates a Echo-State Network (ESN) on the ZC702 FPGA's processing system (SoC). The board creates a TCP server for a client to transfer files over Ethernet and monitor functionality over a USB-UART port. A python script is used to send files and commands easily. The commands (in the form of dummy files with command headers) are used to initialize ESN processing, soft resetting, etc.

## 🛠️ Setup Instructions

1. **Clone the repository:**
   ```bash
   git clone https://github.com/boernerc20/SoC_Project.git
   cd SoC_Project
2. **Import Project into Vitis**
- Launch **Vitis IDE**.
- Go to **File > Import > Import projects from Git > Navigate to `ZC702_File` Project**.
- Select the project and click **Finish**.
- When prompted for the hardware platform:
  - Select `base_ps_wrapper.xsa` from the Vivado export located in the `zc702_soc` folder.
  - Make sure it matches your target hardware.
3. **Open a UART Serial Terminal for Debugging**
- Check what COM port the USB-UART cable is connected to
- Minicom for example (PORT would be the actual UART debugger port):
   ```bash
   minicom -D 'PORT' -b 115200
4. **Set up Ethernet Connection**
- Configure your PC to use a static IP on the same subnet as the board.
- For example:
   ```bash
  PC:      192.168.1.99
  ZC702:   192.168.1.10
  Subnet:  255.255.255.0
4.  **Program the FPGA using Vitis**
- To run the code **Right click project in Explorer > Run As > Launch Hardware**
5. **Monitor TCP Server**
- Should see this message in UART terminal:
   ```bash
   -----ESN Core TCP Server Application-----
  Start PHY autonegotiation
  Waiting for PHY to complete autonegotiation.
  autonegotiation complete
  link speed for phy address 7: 1000
  Configuring default IP 192.168.1.10
  Board IP:       192.168.1.10
  Netmask :       255.255.255.0
  Gateway :       192.168.1.1
  TCP server listening on port 5001
- Confirm Ethernet connection by pinging board (Should see no errors and 0% packet loss):
   ```bash
   ping 192.168.1.20
6. **Run Python Script**
- Navigate to `data` folder.
- Run script:
   ```bash
   ./transmit_data.py
7. **Use Script to Interact with Board**
- See functionality section for more information

## 🧱 Hardware
The hardware and bitstream were synthesized/implemented using Vivado 2023.2. `zc702_soc` houses the Vivado project with the .xsa and .bit files.

**Hardware Block Diagram**
![Block Diagram](images/block_diagram.png)

**Zynq 7000 SoC Design**
![Zynq PS](images/zynq_design.png)
