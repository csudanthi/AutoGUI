#define VNC_PORT_BASE (5900)	//the smallest port number

#define SERVER_BUF_SZ (8*1024*1024)	//size of server-output-data-buffer 
#define CLIENT_BUF_SZ (1*1024)		//size of client-input-data-buffer


//RFB proto version used by AutoGUI
#define AU_USED_VER_MSG_37 "RFB 003.007\n"
#define AU_USED_VER_MSG_38 "RFB 003.008\n"

#define RFBPROTOVER_SZ (12)	//length of RFB proto version message
#define MajorVersion (3)
#define MinorVersion (7)

#define AU_SEC_TYPE_MSG "\1\1"
#define AU_SEC_RESULT "\0\0\0\0"

//Client to Server message type 
#define rfbSetPixelFormat 0
#define rfbSetEncodings 2
#define rfbFramebufferUpdateRequest 3
#define rfbKeyEvent 4
#define rfbPointerEvent 5
#define rfbClientCutText 6

//length of each cts_message head
#define HSZ_SET_PIXEL_FORMAT 20
#define HSZ_SET_ENCODING 4
#define HSZ_FRAME_BUFFER_UPDATE_REQUEST 10
#define HSZ_KEY_EVENT 8
#define HSZ_POINTER_EVENT 6
#define HSZ_CLIENT_CUT_TEXT 8

//Server to Client message type
#define rfbFramebufferUpdate 0
#define rfbSetColourMapEntries 1
#define rfbBell 2
#define rfbServerCutText 3

//length of each stc_message head
#define HSZ_FRAME_BUFFER_UPDATE 4
#define HSZ_SET_COLOUR_MAP_ENTRIES 6
#define HSZ_SERVER_CUT_TEXT 8


//the max length can be read/write socket once
#define SZ_PER_OPT (4*1024*1024)

//Pixel date encoding type
#define rfbEncodingRaw 0

//Key names and Keysym values
#ifndef XK_Control_L
#define XK_Control_L  	0xffe3 
#define XK_Control_R 	0xffe4
#define XK_c			0x00063
#define XK_d			0x00064
#define XK_i			0x00069
#define XK_q			0x00071
#define XK_s			0x00073
#endif

//Default size of captured rectangle
#define RECT_X		(20)
#define RECT_Y		(40)
#define RECT_CX		(780)
#define RECT_CY		(460)

#define BASE_PATH "framein/"




