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

#include <unistd.h>
#include <fcntl.h>
#include <iomanip>

#include "constants.h"
#include "proto.h"

using namespace std;

#define DEBUG

extern ofstream ClientToAU;	//file to write client input
extern ofstream ServerToAU;	//file to write server output
extern unsigned char server_buf[SERVER_BUF_SZ];	//buffer to hold server output data
extern unsigned char client_buf[CLIENT_BUF_SZ];	//buffer to hold client input data
extern unsigned char *sbuf_ptr;
extern uint32_t retnum;
extern uint32_t next_len;

extern unsigned char *cbuf_ptr;
