## 8-bit Atari (or any other retro console) and wireless pad through esp32 and btstack?   

Seems possible.  
Inspired by USB_Host_Shield_2.0 project for Arduino I would like to give it a try ;) 

Right now it's just something for a good start. If you set master mac address to 
your ESP's BT base mac +1 or +2 depending on the number of universal MAC 
addresses defined in your sdkconfig (with sixpair tool, provided) the pad will try 
to connect to ESP32 though BT but doesn't handle the connection correctly yet.
After small modification of btstack/src/hci.c it looks like that:

l2cap:unknown(04) #bluetooth.h HCI_EVENT_CONNECTION_REQUEST  
l2cap:unknown(04)  
l2cap:unknown(0f) #bluetooth.h ATT_ERROR_INSUFFICIENT_ENCRYPTION  
l2cap:unknown(0f)  
l2cap:unknown(6e) #btstack_defines.h L2CAP_LOCAL_CID_DOES_NOT_EXIST  
l2cap:unknown(6e)  
l2cap:unknown(12) #bluetooth.h HCI_EVENT_ROLE_CHANGE  
l2cap:unknown(12)  
[00:00:25.635] LOG -- hci.c.2066: Connection_complete (status=0) 00:1B:FB:EB:FD:55  
[00:00:25.635] LOG -- hci.c.2079: New connection: handle 129, 00:1B:FB:EB:FD:55  
[00:00:25.638] LOG -- hci.c.3983: BTSTACK_EVENT_NR_CONNECTIONS_CHANGED 1  
l2cap:unknown(61) #btstack_defines.h BTSTACK_EVENT_NR_CONNECTIONS_CHANGED  
l2cap:unknown(61)   
l2cap:unknown(03) #btstack_defines.h HID_SUBEVENT_CAN_SEND_NOW  
l2cap:unknown(03)  
l2cap:unknown(0f) #bluetooth.h ATT_ERROR_INSUFFICIENT_ENCRYPTION  
l2cap:unknown(0f)  
[00:00:25.654] LOG -- hci.c.2128: HCI_EVENT_READ_REMOTE_SUPPORTED_ FEATURES_COMPLETE, bonding flags 2, eSCO 0  
l2cap:unknown(0b) #bluetooth.h SM_REASON_DHKEY_CHECK_FAILED             
l2cap:unknown(0b)  
l2cap:unknown(6e) #btstack_defines.h L2CAP_LOCAL_CID_DOES_NOT_EXIST  

   


  
