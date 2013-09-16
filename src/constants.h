#define VNC_PORT_BASE (5900)	//the smallest port number

#define SERVER_BUF_SZ (8*1024*1024)	//size of server-output-data-buffer 
#define CLIENT_BUF_SZ (1*1024)		//size of client-input-data-buffer

#define RFBPROTOVER_SZ (12)	//length of RFB proto version message

//RFB proto version used by AutoGUI
#define AU_USED_VER_MSG "RFB 003.008\n"
#define MajorVersion (3)
#define MinorVersion (7)

#define AU_SEC_TYPE 1	//default secure type for AutoGUI



//boolean type for AutoGUI
typedef int AU_BOOL;
#ifndef True
#define False (0)
#define True (1)
#endif


