#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <netinet/tcp.h>

#include <pthread.h>

#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <iomanip>

#include "constants.h"
#include "proto.h"
using namespace std;

//#define DEBUG
#define ANALYZE_PKTS   //print out the packages which AutoGUI intercepted.
//#define FORCE_UPDATE   //force the vnc-server update the framebuffer in non-incremental mode

extern ofstream ClientToAU;	//file to write client input
extern ofstream ServerToAU;	//file to write server output
extern unsigned char server_buf[SERVER_BUF_SZ];	//buffer to hold server output data
extern unsigned char client_buf[CLIENT_BUF_SZ];	//buffer to hold client input data
extern unsigned char *sbuf_ptr;
extern unsigned char *cbuf_ptr;
extern uint32_t CurPixelData[RECT_CY+20][RECT_CX+20];	//current pixel data in default rectangle size
extern uint32_t CapPixelData[RECT_CY+20][RECT_CX+20];	//captured pixel data in default rectangle size
extern AU_BOOL UpdateFlag;

extern uint32_t retnum;		//bytes raed from socket
extern uint32_t next_len;	//the length will be received next
extern struct timeval FrameBegin;
extern struct timeval FrameEnd;

extern uint8_t button_mask; //the mask state of pointerevent.
extern AU_BOOL CtrlPressed; //deal with combined keystrokes
extern AU_BOOL Recording;   //flag of whether AutoGUI is capturing or not
extern AU_BOOL Replaying;   //flag of whether AutoGUI is replaying or not
extern uint32_t EventCnt;
extern uint32_t FrameCnt; 	

extern pthread_mutex_t mutex; 	
extern ofstream log;			
extern ofstream RecordFile;		
extern ifstream ReplayFile;		
extern ofstream PixelThres;		
extern ifstream PixelThresR;	
extern ofstream RectFramePixel;	
extern ifstream RectFramePixelR;
extern char pathRectFramePixel[1024];
extern float *thres_list;
extern uint32_t thres_list_len;
extern float RfbFrameThres;	

extern ServerInitMsg si;		//initialization message received from vnc-server
extern unsigned char *desktopName;
extern unsigned char ConstName[];
extern unsigned char *p_si;
extern uint8_t AU_SEC_TYPE;		//default secure type(no authentication) for AutoGUI
extern uint8_t AU_SHARED_FLAG; 	//default shared-flag(exclusive) for AutoGUI
extern AU_BOOL FirstInput;		//label for the first input while recording/replaying

//the max version of RFB supported by vnc-server
extern uint32_t server_major, server_minor;

#ifndef TCP_ESTABLISHED 
#define TCP_ESTABLISHED 1
#endif
