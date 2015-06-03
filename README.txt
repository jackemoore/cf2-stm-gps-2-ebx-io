#Crazyflie 2.0 Firmware compatible with Cfclient-Gps-2 - Alamanc

This project contains the source code for the Crazyflie 2.0 firmware with GPS

Compatible with GPS receiver located on Ublox Max M8C Pico Breakout
Receiver connected to cf2 deck (expansion) port uart (RX1/TX1 on left side)
Serial communication uart baud rate 9600 bits/sec
Receiver message output rate - 1 Hz

Currently, cf2 looks for nav-pvt messages from the gps receiver and handles
passing this navigation data to cfclient.  In addition, ebx messages generated
within cfclient and transferred to cf2 are passed on to the gps without
alteration.  At present there is no flowcontrol with the gps.  

The ebx-messages being sent to the gps receiver each contain a checksum and
this is used to verify message integrity prior to outputting it to the receiver.
If the checksum doesn't add up, the cf2 requests cfclient to repeat the entire
data block.  When the cf2 is expecting a transfer from cfclient, it activates a
watchdog timer and if a time out occurs, timeout >= M2T(4000), it requests a
repeat of the last packet.  Duplicate data block packets should not naturally
occur, and the cf2 discards them..  Unfortunately, unintended duplicate
handshaking packets cannot be as easily detected and can disrupt flowcontrol
coordination.

Modules added to cf2 include: drivers/interface/uart_extgps.h, ubx.h;
drivers/src/uart_extgps.c; modules/interface/ablock.h;
modules/src/ablock.c.

Modules modified in cf2 include: drivers/src/nvic.c; modules/interface/crtp.h;
modules/src/comm.c, system.c; parent-directory/makefile.

Currently, cf2 debug messages appear in the Console Tab in cfclient.

The watchdog timer function is located in module ablock.c and is clocked at a
1 Hz rate off of receiving gps nav-pvt messages within module uart.extgps.c.

Alamanc message transfers are initiated by cfclient in the AssistNow Tab by
pressing the push bar.  This triggers a handshaking packet "<S>\n" to be sent
to ablock.c to start things off.  When alblock.c receives a packet "<E>\n"
(normal termination) or an "<X>\n" (abormal termination), the almanac transfer
session is terminated.  ablock.c can terminate the acitivity when retry limits
for lost packets or bad messages are exceeded.  In which case it sends a "<X>\n"
abnormal termination to cfclient.  Only cfclient can start an almanac transfer
session.