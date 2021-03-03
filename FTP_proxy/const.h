#ifndef _CONST_H_
#define _CONST_H_

#include "./kernelTypes.h"
#pragma comment (lib, "Ws2_32.lib")
//#define D_TESTING

const uint8 ProxyControlAuto_TYPE_ID = 0x00;

const uint8 ProxyControlAuto_MBX_ID = 0x00;

// Proxy control msges
const uint8 MSG_Set_Lissener_And_Connect = 0x0001;
const uint8 MSG_Proxy_Control_Auto_Connecting = 0x0002;
const uint8 MSG_Connected_To_Server = 0x0003;

#define DEFAULT_BUFLEN 512


#define TIMER1_ID 0
#define TIMER1_COUNT 10
#define TIMER1_EXPIRED 0x20


#endif //_CONST_H_