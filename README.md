# COMP3601-Project: Audio Networking with KV260 FPGA 

**Description:**
This project takes in data from an i2s microphone and streams it in real time over an ethernet connection via UDP. 

**Team Members:**
1. Muzhi Wang
2. Shourya Saklecha
3. James Milosavljevic
4. Diresh Roshandeivendra

**Hardware:**
1. Kria KV260 FPGA with ARM processor.
2. ADAFruit i2s MEMs Microphone
3. Ethernet Cable

**Software:**
Xilinx Vivado 2020.1


**File Structure/Navigation:**
This project was divided into 3 milestones. 
1. Milestone 1: Set up the i2s protocol in VHDL. See "i2s.vhd". Other HDL files in the root directory (e.g. FIFO) interface with i2s.vhd.
2. Milestone 2: The main code for this is in the "app/src/main.c". Here, we take in the i2s data from the DMA of the processor and generate a .wav file.
3. Milestone 3: In this milestone, we took in the data from the microphone and sent it over a network in real-time through UDP over ethernet. Server-side code can be found in "app_m4/src/main.c" and client-side code can be found in "app_m4/UDP_client_write.c". All other files are to implement these interfaces.  
 
Our Kria FPGA board acts as a server that transmits the data in real time to our computer which acting as the client, plays the file through VLC. This audio is continuously streamed in real-time, with a static delay equal to the delay between the start of the connection and the file being played on the client. 

**Setup:**
1. Connect the ethernet cable from the board to your PC.
2. Turn off the firewall for both the board and the PC.
3. Use ifconfig eth0 192.168.0.149 to set a static ip address for the board (192.168.0.149).
4. Set up your PC's manual IP assignment to 192.168.0.212 (should work for other local IP addresses as well but we prefer the one we provided).
5. Compile the code for the board - get sample256 binary for the board and run it on the kria board.
6. Compile UDP_client_write.c and run it on your PC
7. Send something from the client then you can receive the voice from the board!
   
**Usage:**
DO THIS AFTER SETUP. 
1. Run sample256 binary file on the board (after you correctly configured the pins of the microphone and loaded the app of the hardware). 
2. Run the compiled code of UDP_client_write.c on your PC.
3. After successfully combining the ports, type something on the PC's terminal.
4. Now the continuous receiving will be displayed and you can now play the wavefile - VLC is suggested for playback
5. To stop receiving the .wav file, just CTRL + C to terminate the execution on both the board and your PC manually.

**Testing and Troubleshooting:**
- When running sample256 and it does not show "listening for incoming messages", use ping 192.168.0.212 on the board (change 192.168.0.212 to the IP address you assigned to your PC), to see if the board can print something out. If it cannot, redo the setup again and double-check the IP on the board and that the IP assignment is manual instead of DHCP.

- When running the compile code of UDP_client_write.c, if you receive errors like "Unable to send message" OR "Error while receiving server's msg", redo the entire setup as well.

- When running the compiled code of UDP_client_write.c, after entering something if you do not recieve "continuously receiving", redo the setup.
