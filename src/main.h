/*
Copyright (c) 2013, Perth Charles
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  Neither the name of the {organization} nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
