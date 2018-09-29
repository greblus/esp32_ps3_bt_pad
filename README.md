## 8-bit Atari (or any other retro console) and wireless pad through esp32 and btstack?   

Seems possible.  
Inspired by USB_Host_Shield_2.0 project for Arduino I would like to give it a try ;) 

Right now it's just something for a good start. If you set master mac address to 
your ESP's BT base mac +1 or +2 depending on the number of universal MAC 
addresses defined in your sdkconfig (with sixpair tool, provided) the pad will try 
to connect to ESP32 though BT but doesn't handle the connection correctly yet:

[00:00:00.376] LOG -- l2cap.c.3401: L2CAP_REGISTER_SERVICE psm 0x11 mtu 100  
[00:00:00.376] LOG -- l2cap.c.3401: L2CAP_REGISTER_SERVICE psm 0x13 mtu 100  
[00:00:00.378] LOG -- rfcomm.c.2301: RFCOMM_REGISTER_SERVICE channel #1 mtu 100 flow_control 1 credits 1  
[00:00:00.387] LOG -- l2cap.c.3401: L2CAP_REGISTER_SERVICE psm 0x3 mtu 65535  
[00:00:00.394] LOG -- l2cap.c.3401: L2CAP_REGISTER_SERVICE psm 0x1 mtu 65535  
[00:00:00.401] LOG -- hci.c.2800: hci_power_control: 1, current mode 0  
[00:00:00.407] LOG -- main.c.204: transport_init  
[00:00:00.411] LOG -- main.c.220: transport_open  
I (416) BTDM_INIT: BT controller compile version [e1a0628]  
I (482) phy: phy_version: 4000, b6198fa, Sep  3 2018, 15:11:06, 0, 0  
[00:00:00.807] LOG -- main.c.251: transport: configure SCO over HCI, result 0x0000  
[00:00:00.808] LOG -- hci.c.3859: BTSTACK_EVENT_STATE 1  
BTstack: execute run loop  
[00:00:00.812] LOG -- hci.c.1902: Manufacturer: 0x0060  
[00:00:00.816] LOG -- hci.c.1816: local name:   
[00:00:00.820] LOG -- hci.c.1605: Received local name, need baud change 0  
[00:00:00.827] LOG -- hci.c.1912: Local supported commands summary 0x3f  
[00:00:00.833] LOG -- hci.c.1864: Local Address, Status: 0x00: Addr: 01:02:03:04:05:08  
[00:00:00.841] LOG -- hci.c.1833: hci_read_buffer_size: ACL size module 1021 -> used 1021, count 9 / SCO size 255, count 4  
[00:00:00.852] LOG -- hci.c.1891: Packet types cc18, eSCO 1  
[00:00:00.856] LOG -- hci.c.1894: BR/EDR support 1, LE support 1  
[00:00:00.866] LOG -- hci.c.3971: BTSTACK_EVENT_DISCOVERABLE_ENABLED 0  
[00:00:00.870] LOG -- hci.c.1844: hci_le_read_buffer_size: size 251, count 10  
[00:00:00.876] LOG -- hci.c.1713: Supported commands 30  
[00:00:00.881] LOG -- hci.c.1851: hci_le_read_maximum_data_length: tx octets 251, tx time 2120 us  
[00:00:00.890] LOG -- hci.c.1857: hci_le_read_white_list_size: size 12  
[00:00:00.896] LOG -- hci.c.1469: hci_init_done -> HCI_STATE_WORKING  
[00:00:00.901] LOG -- hci.c.3859: BTSTACK_EVENT_STATE 2  
BTstack: up and running.  
[00:00:01.521] LOG -- hci.c.1982: Connection_incoming: 00:1B:FB:EB:FD:55, type 1  
[00:00:01.522] LOG -- hci.c.185: create_connection_for_addr 00:1B:FB:EB:FD:55, type ff  
[00:00:01.525] LOG -- hci.c.3292: sending hci_accept_connection_request, remote eSCO 0  
[00:00:01.550] LOG -- hci.c.2006: Connection_complete (status=0) 00:1B:FB:EB:FD:55  
[00:00:01.551] LOG -- hci.c.2019: New connection: handle 128, 00:1B:FB:EB:FD:55  
[00:00:01.554] LOG -- hci.c.3922: BTSTACK_EVENT_NR_CONNECTIONS_CHANGED 1  
[00:00:01.561] LOG -- hci.c.2085: HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE, bonding flags 2, eSCO 0  
[00:00:01.631] LOG -- hci.c.889: Connection closed: handle 0x80, 00:1B:FB:EB:FD:55  
  
