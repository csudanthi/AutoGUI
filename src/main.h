#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <linux/tcp.h>

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

extern uint32_t retnum;		//bytes raed from socket
extern uint32_t next_len;	//the length will be received next

extern ServerInitMsg si;	//initialization message received from vnc-server
extern unsigned char *desktopName;
extern unsigned char ConstName[];
extern unsigned char *p_si;
extern uint8_t AU_SEC_TYPE;	//default secure type(no authentication) for AutoGUI
extern uint8_t AU_SHARED_FLAG; 	//default shared-flag(exclusive) for AutoGUI

//the max version of RFB supported by vnc-server
extern uint32_t server_major, server_minor;

#ifndef TCP_ESTABLISHED 
#define TCP_ESTABLISHED 1
#endif
