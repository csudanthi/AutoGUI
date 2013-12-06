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
// print out the packages which AutoGUI intercepted.
#define ANALYZE_PKTS
// force the vnc-server update the framebuffer in non-incremental mode
//#define FORCE_UPDATE

// file to write client input
extern ofstream ClientToAU;
// file to write server output
extern ofstream ServerToAU;
// buffer to hold server output data
extern unsigned char server_buf[SERVER_BUF_SZ];
// buffer to hold client input data
extern unsigned char client_buf[CLIENT_BUF_SZ];
extern unsigned char *sbuf_ptr;
extern unsigned char *cbuf_ptr;
// current pixel data in default rectangle size
extern uint32_t CurPixelData[RECT_CY+20][RECT_CX+20];
// captured pixel data in default rectangle size
extern uint32_t CapPixelData[RECT_CY+20][RECT_CX+20];
extern AU_BOOL UpdateFlag;

// bytes raed from socket
extern uint32_t retnum;
// the length will be received next
extern uint32_t next_len;
extern struct timeval FrameBegin;
extern struct timeval FrameEnd;

// the mask state of pointerevent.
extern uint8_t button_mask;
// deal with combined keystrokes
extern AU_BOOL CtrlPressed;
// flag of whether AutoGUI is capturing or not
extern AU_BOOL Recording;
// flag of whether AutoGUI is replaying or not
extern AU_BOOL Replaying;
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

// initialization message received from vnc-server
extern ServerInitMsg si;
extern unsigned char *desktopName;
extern unsigned char ConstName[];
extern unsigned char *p_si;
// default secure type(no authentication) for AutoGUI
extern uint8_t AU_SEC_TYPE;
// default shared-flag(exclusive) for AutoGUI
extern uint8_t AU_SHARED_FLAG;
// label for the first input while recording/replaying
extern AU_BOOL FirstInput;

// the max version of RFB supported by vnc-server
extern uint32_t server_major, server_minor;

#ifndef TCP_ESTABLISHED
#define TCP_ESTABLISHED 1
#endif
