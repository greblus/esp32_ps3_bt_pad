// Based on https://github.com/dsky7/PIC/tree/master/sbxbt_ps3
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "btstack_config.h"
#include "btstack.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define FORMAT_RCB3                  1
#define USE_SPP_SERVER               1
#define MAX_ATTRIBUTE_VALUE_SIZE   300
#define HID_BUFFERSIZE              50
#define OUTPUT_REPORT_BUFFER_SIZE   48
#define  CREDITS                    10
#define STICK_DEADZONE              10 
#define STICK_RESOLUTION             0
#define STICK_MINIMUM_VALUE          0
#define STICK_MAXIMUM_VALUE        127 
//#define STICK_MAXIMUM_VALUE      255
#define STICK_NEUTRAL_VALUE         64    
//#define STICK_NEUTRAL_VALUE      128

#define OUT_DIGITAL_UP          0x0001
#define OUT_DIGITAL_DOWN        0x0002
#define OUT_DIGITAL_RIGHT       0x0004
#define OUT_DIGITAL_LEFT        0x0008
#define OUT_DIGITAL_TRIANGLE    0x0010
#define OUT_DIGITAL_CROSS       0x0020
#define OUT_DIGITAL_CIRCLE      0x0040
#define OUT_DIGITAL_RECTANGLE   0x0100
#define OUT_DIGITAL_L1          0x0200
#define OUT_DIGITAL_L2          0x0400
#define OUT_DIGITAL_R1          0x0800
#define OUT_DIGITAL_R2          0x1000
#define OUT_DIGITAL_START       0x0003
#define OUT_DIGITAL_SELECT      0x000C
#define OUT_DIGITAL_L3          0x0000
#define OUT_DIGITAL_R3          0x0000
#define OUT_DIGITAL_PS          0x0000
#define get_pairen() (1)
#define READ_BT_16( buffer, pos) ( ((uint16_t) buffer[pos]) | (((uint16_t)buffer[pos+1]) << 8))

typedef unsigned char           BYTE;
typedef unsigned short int      WORD;

static WORD sixaxis_interrupt_channel_id=0;
bd_addr_t addr_global;
static uint8_t startup_state=0;
#if USE_SPP_SERVER
static uint8_t    rfcomm_send_credit=CREDITS;
static uint8_t rfcomm_channel_nr = 1;
static uint16_t rfcomm_channel_id;
static uint8_t __attribute__ ((aligned(2))) spp_service_buffer[128];
#endif
char lineBuffer[50];

const BYTE OUTPUT_REPORT_BUFFER[] = {
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x02 /*0x00*/,  //140201 LED1
    0xff, 0x27, 0x10, 0x00, 0x32,
    0xff, 0x27, 0x10, 0x00, 0x32,
    0xff, 0x27, 0x10, 0x00, 0x32,
    0xff, 0x27, 0x10, 0x00, 0x32,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define BUTTON_STATE_INIT     0
#define  ANALOG_STATE_INIT    0x80

WORD sixaxis_control_channel_id=0;
WORD sixaxis_button_state=BUTTON_STATE_INIT;
BYTE sixaxis_lx=ANALOG_STATE_INIT;
BYTE sixaxis_ly=ANALOG_STATE_INIT;
BYTE sixaxis_rx=ANALOG_STATE_INIT;
BYTE sixaxis_ry=ANALOG_STATE_INIT;


static const char * addr_global_string = "00:1B:FB:EB:FD:55";

static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t l2cap_control_packet_callback_registration;
static btstack_packet_callback_registration_t l2cap_interrupt_packet_callback_registration;

uint8_t getsum(void *buf,size_t size)
{
   const uint8_t  *p=(uint8_t*)buf;
   uint8_t        ret=0;

   for(;size;size--){
      ret+=*p++;
   }
   return ret;
}

uint8_t conv_analog(uint8_t ai)
{
   uint8_t ret;

#if   STICK_NEUTRAL_VALUE==64
   ai>>=1;
#endif
   if(ai>(STICK_NEUTRAL_VALUE+STICK_DEADZONE)) {
   #if   USE_STICK_FOR_DIGITAL
      ret=STICK_MAXIMUM_VALUE;
   #else
      #if STICK_RESOLUTION
         ret=((ai+(STICK_RESOLUTION-1))/STICK_RESOLUTION)*STICK_RESOLUTION-1;
      #else
         ret=ai;
      #endif
      #if   STICK_MAXIMUM_VALUE<255
      if(ret>STICK_MAXIMUM_VALUE){
         ret=STICK_MAXIMUM_VALUE;
      }
      #endif
   #endif
   } else if(ai<(STICK_NEUTRAL_VALUE-STICK_DEADZONE)) {
   #if   USE_STICK_FOR_DIGITAL
      ret=STICK_MINIMUM_VALUE;
   #else
      #if STICK_RESOLUTION
         ret=((ai/STICK_RESOLUTION)*STICK_RESOLUTION;
      #else
         ret=ai;
      #endif
      #if   STICK_MINIMUM_VALUE
         if(ret<STICK_MINIMUM_VALUE){
            ret=STICK_MINIMUM_VALUE;
         }
      #endif
   #endif
   } else {
      ret=STICK_NEUTRAL_VALUE;   
   }

   return ret;
}

void out_button_state(void)
{
   int   pos=0;
#if FORMAT_RCB3
   lineBuffer[pos++]=0x80; 
#elif FORMAT_RCB4
   lineBuffer[pos++]=0x0D;
   lineBuffer[pos++]=0x00;
   lineBuffer[pos++]=0x02;
   lineBuffer[pos++]=0x50;
   lineBuffer[pos++]=0x03;
   lineBuffer[pos++]=0x00;
#elif FORMAT_RPU
   lineBuffer[pos++]=0x4B;
   lineBuffer[pos++]=0xFF;
#else
#error   "Unknown format"
#endif
#if FORMAT_RCB3
   lineBuffer[pos++]=(sixaxis_button_state>>8)&0xff;
   lineBuffer[pos++]=(sixaxis_button_state>>0)&0xff;
   //xyxy
   lineBuffer[pos++]=conv_analog(sixaxis_lx);
   lineBuffer[pos++]=conv_analog(sixaxis_ly);
   lineBuffer[pos++]=conv_analog(sixaxis_rx);
   lineBuffer[pos++]=conv_analog(sixaxis_ry);
   lineBuffer[pos]=(getsum(lineBuffer,pos)&0x7f);  
   pos++;
#elif FORMAT_RCB4
   lineBuffer[pos++]=(sixaxis_button_state>>8)&0xff;
   lineBuffer[pos++]=(sixaxis_button_state>>0)&0xff;
   //yxyx
   lineBuffer[pos++]=conv_analog(0xFF-sixaxis_ly); 
   lineBuffer[pos++]=conv_analog(sixaxis_lx);   
   lineBuffer[pos++]=conv_analog(0xFF-sixaxis_ry); 
   lineBuffer[pos++]=conv_analog(sixaxis_rx);   
   lineBuffer[pos]=(getsum(lineBuffer,pos)&0xFF);
   pos++;
#elif FORMAT_RPU
   lineBuffer[pos++]=((sixaxis_button_state>>8)&0xff)^0xff;
   lineBuffer[pos++]=((sixaxis_button_state>>0)&0xff)^0xff;
   lineBuffer[pos++]=conv_analog(sixaxis_rx);      
   lineBuffer[pos++]=conv_analog(sixaxis_ry);   
   lineBuffer[pos++]=conv_analog(sixaxis_lx);
   lineBuffer[pos++]=conv_analog(sixaxis_ly);
   lineBuffer[pos++]=0xF3;             
#endif

   return;
}

#if FORMAT_RCB3
   const uint8_t neutral_packet[]={0x80,0x00,0x00,0x40,0x40,0x40,0x40,};
#elif FORMAT_RCB4
   const uint8_t neutral_packet[]={0x0D,0x00,0x02,0x50,0x03,0x00,0x00,0x00,0x40,0x40,0x40,0x40,};  
#elif FORMAT_RPU
   const uint8_t neutral_packet[]={0x4B,0xFF,0xFF,0xFF,0x80,0x80,0x80,0x80,0xF3};   
   const uint8_t disconnect_packet[]={0x4B,0xFF,0xFF,0xFF,0x80,0x80,0x80,0x80,0xFF};   
   const uint8_t connect_packet[]={0x4B,0xFF,0xFF,0xFF,0x80,0x80,0x80,0x80,0xFB};   
#endif

void init_button_state(void)
{
   sixaxis_button_state=BUTTON_STATE_INIT;
   sixaxis_lx=ANALOG_STATE_INIT;
   sixaxis_ly=ANALOG_STATE_INIT;
   sixaxis_rx=ANALOG_STATE_INIT;
   sixaxis_ry=ANALOG_STATE_INIT;

#if   FORMAT_RPU
   memcpy(lineBuffer,disconnect_packet,sizeof(disconnect_packet));
#if   SEND_ON_DIFFERENT_DATA || NEUTRAL_DATA_SUPPRESS
   memcpy(lastsendbuf,disconnect_packet,sizeof(disconnect_packet));
#endif
   UART1PutBuf(lineBuffer,sizeof(disconnect_packet));
#else
   out_button_state();
#endif
}

static int enable_sixaxis(uint16_t channel)
{
    uint8_t cmd_buf[6];
    cmd_buf[0] = 0x53;// HID BT Set_report (0x50) | Report Type (Feature 0x03)
    cmd_buf[1] = 0xF4;// Report ID
    cmd_buf[2] = 0x42;// Special PS3 Controller enable commands
    cmd_buf[3] = 0x03;
    cmd_buf[4] = 0x00;
    cmd_buf[5] = 0x00;

   return l2cap_send(channel,cmd_buf,6);
}

static int set_sixaxis_led(uint16_t channel,int led)
{
   memcpy(lineBuffer+2,OUTPUT_REPORT_BUFFER,OUTPUT_REPORT_BUFFER_SIZE);
    lineBuffer[0] = 0x52;// HID BT Set_report (0x50) | Report Type (Output 0x02)
    lineBuffer[1] = 0x01;// Report ID
   //lineBuffer[11] |= (uint8_t)(((uint16_t)led & 0x0f) << 1);    

   return l2cap_send(channel,(uint8_t*)lineBuffer,HID_BUFFERSIZE);
}

#if   USE_DUALSHOCK3_RUMBLE
static int set_sixaxis_rumble(uint16_t channel,int rumble)
{
   memcpy(lineBuffer+2,OUTPUT_REPORT_BUFFER,OUTPUT_REPORT_BUFFER_SIZE);
    lineBuffer[0] = 0x52;// HID BT Set_report (0x50) | Report Type (Output 0x02)
    lineBuffer[1] = 0x01;// Report ID
   lineBuffer[3] = (rumble==1)?10:0;      //rightDuration
   lineBuffer[4] = (rumble==1)?0xff:0;    //rightPower
   lineBuffer[5] = (rumble==2)?10:0;      //leftDuraion
   lineBuffer[6] = (rumble==2)?0xff:0;    //leftPower

   return l2cap_send(channel,(uint8_t*)lineBuffer,HID_BUFFERSIZE);
}
#endif

void sixaxis_process_packet(BYTE *hid_report,WORD size)
{
   WORD  button_state;

   if(hid_report[0]!=0x01 || hid_report[1]!=0x00){
      return;
   }

   //digital state
   button_state=0;
   if(hid_report[2]&0x80){
      button_state|=OUT_DIGITAL_LEFT;
   }
   if(hid_report[2]&0x40){
      button_state|=OUT_DIGITAL_DOWN;
   }
   if(hid_report[2]&0x20){
      button_state|=OUT_DIGITAL_RIGHT;
   }
   if(hid_report[2]&0x10){
      button_state|=OUT_DIGITAL_UP;
   }
   if(hid_report[2]&0x08){
      button_state|=OUT_DIGITAL_START;
   }
   if(hid_report[2]&0x04){
      button_state|=OUT_DIGITAL_R3;
   }
   if(hid_report[2]&0x02){
      button_state|=OUT_DIGITAL_L3;
   }
   if(hid_report[2]&0x01){
      button_state|=OUT_DIGITAL_SELECT;
   }
   if(hid_report[3]&0x80){
      button_state|=OUT_DIGITAL_RECTANGLE;
   }
   if(hid_report[3]&0x40){
      button_state|=OUT_DIGITAL_CROSS;
   }
   if(hid_report[3]&0x20){
      button_state|=OUT_DIGITAL_CIRCLE;
   }
   if(hid_report[3]&0x10){
      button_state|=OUT_DIGITAL_TRIANGLE;
   }
   if(hid_report[3]&0x08){
      button_state|=OUT_DIGITAL_R1;
   }
   if(hid_report[3]&0x04){
      button_state|=OUT_DIGITAL_L1;
   }
   if(hid_report[3]&0x02){
      button_state|=OUT_DIGITAL_R2;
   }
   if(hid_report[3]&0x01){
      button_state|=OUT_DIGITAL_L2;
   }
   if(hid_report[4]&0x01){
      button_state|=OUT_DIGITAL_PS;
   }
   sixaxis_button_state=button_state;

   //analog state
   sixaxis_lx=hid_report[ 6];
   sixaxis_ly=hid_report[ 7];
   sixaxis_rx=hid_report[ 8];
   sixaxis_ry=hid_report[ 9];

   out_button_state();
   return;
}

static uint8_t sixaxis_init_state=0;

static void packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
   bd_addr_t   event_addr;
#if   USE_SPP_SERVER
    uint8_t    rfcomm_channel_nr;
    uint8_t    rfcomm_channel_id_temp;
    uint16_t   mtu;
#endif

   switch (packet_type) {
      case HCI_EVENT_PACKET:
         switch (packet[0]) {
            case BTSTACK_EVENT_STATE:
               // bt stack activated, get started - set local name
               if (packet[2] == HCI_STATE_WORKING) {
#ifndef  __DEBUG
                  if(
                     !addr_global[0]
                  && !addr_global[1]
                  && !addr_global[2]
                  && !addr_global[3]
                  && !addr_global[4]
                  && !addr_global[5]
                  ){
                     //bd address failed -> reset
                     printf("bs address failed -> Reset() was here");
                     //return;
                  }
#endif
                  sprintf(lineBuffer,"local_name", "-%02x%02x%02x%02x%02x%02x",
                     addr_global[0],
                     addr_global[1],
                     addr_global[2],
                     addr_global[3],
                     addr_global[4],
                     addr_global[5]
                  );
                  #if STANDALONE_PAIRING
                  save_local_bluetooth_address(addr_global);
                  #endif

                  printf("local name:%s\r\n",lineBuffer);
#ifndef  __DEBUG
                  vTaskDelay(100 / portTICK_PERIOD_MS);
#endif
                        hci_send_cmd(&hci_write_local_name, lineBuffer);
               }
               break;
            
            case HCI_EVENT_COMMAND_COMPLETE:
               if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_read_bd_addr)){
                        reverse_bd_addr(&packet[6], event_addr);
                        printf("BD-ADDR: %s\n\r", bd_addr_to_str(event_addr));
#if !USE_SPP_SERVER
                  startup_state=1;
#endif
                        break;
                    }
#if   USE_SPP_SERVER
               if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_write_local_name)){
                   hci_send_cmd(&hci_write_class_of_device, 0);
                        break;
                    }
               if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_write_class_of_device)){
                  if(get_pairen()){
#if   DELETE_STORED_LINK_KEY
hci_send_cmd(&hci_delete_stored_link_key,addr_global,1);
#endif
                     gap_discoverable_control(1);
                  }
                  startup_state=1;
                  hci_send_cmd(&hci_write_authentication_enable, 1);
                        break;
               }
               if (HCI_EVENT_IS_COMMAND_COMPLETE(packet, hci_write_authentication_enable)){
                  gap_discoverable_control(1);
                break;
               }
#endif
                    break;
            case HCI_EVENT_LINK_KEY_REQUEST:
               // deny link key request
                    printf("Link key request\n\r");
                    reverse_bd_addr(&packet[2], event_addr);
               hci_send_cmd(&hci_link_key_request_negative_reply, &event_addr);
               break;
               
            case HCI_EVENT_PIN_CODE_REQUEST:
               // inform about pin code request
                    reverse_bd_addr(&packet[2], event_addr);
                    printf("Pin code request - using PIN_CODE_DEFAULT\n");
               hci_send_cmd(&hci_pin_code_request_reply, &event_addr, 4, "0000");
               break;
#if   USE_SPP_SERVER
                case RFCOMM_EVENT_INCOMING_CONNECTION:
               // data: event (8), len(8), address(48), channel (8), rfcomm_cid (16)
               reverse_bd_addr(&packet[2], event_addr); 
               rfcomm_channel_nr = packet[8];
               rfcomm_channel_id_temp = READ_BT_16(packet, 9);
               printf("RFCOMM channel %u requested for %s\n\r", rfcomm_channel_nr, bd_addr_to_str(event_addr));
                    rfcomm_accept_connection(rfcomm_channel_id_temp);
               break;
               
            case RFCOMM_EVENT_CHANNEL_OPENED:
               // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
               if (packet[2]) {
                  printf("RFCOMM channel open failed, status %u\n\r", packet[2]);
               } else {
                  rfcomm_channel_id = READ_BT_16(packet, 12);
                  mtu = READ_BT_16(packet, 14);
                  printf("\n\rRFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n\r", rfcomm_channel_id, mtu);
               }
               break;
                    
                case RFCOMM_EVENT_CHANNEL_CLOSED:
                    rfcomm_channel_id = 0;
                    break;
#endif
                default:
                    break;
         }
            break;
#if   USE_SPP_SERVER
        case RFCOMM_DATA_PACKET:
            // hack: truncate data (we know that the packet is at least on byte bigger
            //packet[size] = 0;
            //printf( (const char *) packet);
          //led1_on_mac();
         for(;size--;){
            printf("%c", (*packet++));
         }
         printf("\n");
         if(!--rfcomm_send_credit){
               rfcomm_send_credit = CREDITS;
              rfcomm_grant_credits(rfcomm_channel_id, rfcomm_send_credit);
         }
         break;
#endif
        default:
            break;
   }
}

static void l2cap_control_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
   if (packet_type == HCI_EVENT_PACKET){
      switch(packet[0]){
      case L2CAP_EVENT_INCOMING_CONNECTION:
         printf("L2CAP_EVENT_INCOMING_CONNECTION\n");
         l2cap_accept_connection(channel);
         break;
      case L2CAP_EVENT_CHANNEL_OPENED:
           if (packet[2]) {
               printf("control Connection failed\n\r");
               return;
          }
         printf("control Connected\n\r");
         sixaxis_control_channel_id=channel;
         break;
      case L2CAP_EVENT_CHANNEL_CLOSED:
         printf("L2CAP_CHANNEL_CLOSED\n");
         sixaxis_control_channel_id=0;
         sixaxis_init_state=0;
         init_button_state();
         break;
      case DAEMON_EVENT_L2CAP_CREDITS:
         switch(sixaxis_init_state){
         case  1:
            sixaxis_init_state++;
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            set_sixaxis_led(channel,1);
#if   FORMAT_RPU
            memcpy(lineBuffer,connect_packet,sizeof(connect_packet));
#if   SEND_ON_DIFFERENT_DATA || NEUTRAL_DATA_SUPPRESS
            memcpy(lastsendbuf,connect_packet,sizeof(connect_packet));
#endif
            UART1PutBuf(lineBuffer,sizeof(connect_packet));
#endif
            break;
         default:
            break;
         }
         break;
      default:
         printf("l2cap:unknown(%02x)\n",packet[0]);
         break;
      }  
   }
}

static void l2cap_interrupt_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
   if (packet_type == HCI_EVENT_PACKET){
      switch(packet[0]){
      case L2CAP_EVENT_INCOMING_CONNECTION:
         printf("L2CAP_EVENT_INCOMING_CONNECTION\n");
         l2cap_accept_connection(channel);
         break;
      case L2CAP_EVENT_CHANNEL_OPENED:
           if (packet[2]) {
               printf("interrupt Connection failed\n\r");
               return;
          }
         printf("interrupt Connected\n\r");
         sixaxis_interrupt_channel_id=channel;
         break;
      case L2CAP_EVENT_CHANNEL_CLOSED:
         printf("L2CAP_CHANNEL_CLOSED\n");
         sixaxis_interrupt_channel_id=0;
         sixaxis_init_state=0;
         init_button_state();
         break;
      case DAEMON_EVENT_L2CAP_CREDITS:
         switch(sixaxis_init_state){
         case  0:
            sixaxis_init_state++;
            enable_sixaxis(sixaxis_control_channel_id);
            break;
         default:
            break;
         }        break;
      default:
         printf("l2cap:unknown(%02x)\n",packet[0]);
         break;
      }  
   }
   if (packet_type == L2CAP_DATA_PACKET && size && packet[0]==0xa1){
      sixaxis_process_packet(packet+1,size-1);
   }
}

static void hid_host_setup(void){

    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    
    l2cap_control_packet_callback_registration.callback = &l2cap_control_packet_handler;
    hci_add_event_handler(&l2cap_control_packet_callback_registration);
    l2cap_interrupt_packet_callback_registration.callback = &l2cap_interrupt_packet_handler; ;
    hci_add_event_handler(&l2cap_interrupt_packet_callback_registration);

    // Disable stdout buffering
    setbuf(stdout, NULL);
}

int btstack_main(int argc, const char * argv[]){

    (void)argc;
    (void)argv;

    uint8_t nmac[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    esp_base_mac_addr_set(nmac);
    
    init_button_state();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    hid_host_setup();
    l2cap_init();
    l2cap_register_packet_handler(packet_handler);
    l2cap_register_service(l2cap_control_packet_handler, PSM_HID_CONTROL, 160, LEVEL_2);
    l2cap_register_service(l2cap_interrupt_packet_handler, PSM_HID_INTERRUPT, 160, LEVEL_2);
#if USE_SPP_SERVER    
    rfcomm_init();
    //rfcomm_register_packet_handler(packet_handler);
    rfcomm_register_service_with_initial_credits(packet_handler, rfcomm_channel_nr, 160, 1);  // reserved channel, mtu=100, 1 credit
     
    sdp_init();
 
    memset(spp_service_buffer, 0, sizeof(spp_service_buffer));
    spp_create_sdp_record(spp_service_buffer, 0x10001, 1, "SPP");
    sdp_register_service(spp_service_buffer);
#endif
     // parse human readable Bluetooth address
     sscanf_bd_addr(addr_global_string, addr_global);
 
     // Turn on the device 
     hci_power_control(HCI_POWER_ON);
     return 0;
}

