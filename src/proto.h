/* boolean type for AutoGUI */
typedef uint32_t AU_BOOL;
#ifndef True
#define False (0)
#define True (1)
#endif

/* Bitmap struct for .bmp file
 * This format can be found in
 * http://en.wikipedia.org/wiki/BMP_file_format#Example_1
 */
#pragma pack(push)
#pragma pack(1)
typedef struct _BMP_HEAD{
    //BMP header
    unsigned char id_field[2];
    // bmp_size = 14 + 40 + 800*480*4 = 1536054 = 0x177036
    uint32_t bmp_size;
    uint32_t pad_1;
    // 54 bytes
    uint32_t data_offset;
    //DIB header
    uint32_t dib_size;
    // 800
    uint32_t width;
    // 480 (the pixel order used is "top-to-bottom",
    // so the stored value is -480)
    int32_t height;
    uint16_t plane_cnt;
    // 32bits, 4bytes
    uint16_t bitperpixel;
    // no pixel array compression used
    uint32_t compress;
    // 800*400*4
    uint32_t data_size;
    // default = 2835 pixels/meter
    uint32_t h_resolution;
    // default = 2835 pixels/meter
    uint32_t v_resolution;
    uint32_t palette;
    uint32_t important_color;
}BMP_HEAD;
#pragma pack(pop)


#define INIT_BMP(X) BMP_HEAD X = {  \
    {'B', 'M'},                     \
    14 + 40 + 800*480*4,            \
    0,                              \
    54,                             \
    40,                             \
    800,                            \
    -480,                           \
    1,                              \
    32,                             \
    0,                              \
    800*480*4,                      \
    2835,                           \
    2835,                           \
    0,                              \
    0                               \
}


/* Macros for endian swapping. */
#define Swap16(s) ( (((s) & 0xff) << 8) | (((s) >> 8) & 0xff) )
#define Swap32(s) ( ((s) >> 24) | \
                    ((s) & 0x00ff0000) >> 8 | \
                    ((s) & 0x0000ff00) << 8 | \
                    ((s) << 24) \
                  )
typedef struct _RectUpdate{
    uint16_t x_pos;
    uint16_t y_pos;
    uint16_t width;
    uint16_t height;
    uint32_t encoding_type;
}RectUpdate;
#define SZ_RECT_UPDATE ( 8+4 )

/* Structure used to pass parameter to child-thread */
typedef struct _SocketSet{
    uint32_t SocketToClient;  // the socket which received data from vnc-client
    uint32_t SocketToServer;  // the socket which received data from vnc-server
}SocketSet;
#define SZ_SOCKET_SET (8)

/* Structure used to specify pixel format */
typedef struct _PixelFormat{
    uint8_t bitsPerPixel;    /*8,16,32 only*/
    uint8_t depth;        /*8 to 32*/
    uint8_t bigEndian;        /*True or False*/
    uint8_t trueColour;        /*True or False*/
    /* the following field iare only meaningful if trueColor is true */
    uint16_t redMax;
    uint16_t greenMax;
    uint16_t blueMax;
    uint8_t redShift;
    uint8_t greenShift;
    uint8_t blueShift;
    uint8_t pad1;
    uint16_t pad2;
}PixelFormat;
#define SZ_PIXEL_FORMAT (16)

/* Server initialization message
 *
 * Response to the client initialization message
 * This tells the client the width and height of the server's framebuffer,
 * its pixel format and the name associated with the desktop
 */
typedef struct _ServerInitMsg {
    uint16_t framebufferWidth;
    uint16_t framebufferHeight;
    /* the server preferredd pixel format */
    PixelFormat format;
    uint32_t nameLength;
    /* followed by char name[nameLength] */
} ServerInitMsg;
#define SZ_SERVER_INIT_MSG (8 + SZ_PIXEL_FORMAT)

/* Types of packets that can be received/write from/to capture file */
enum EventPkt {
    rfbPkt,
    timewaitPkt,
    timesyncPkt,
    framewaitPkt,
    ddelayPkt,
    chkpointPkt,
    exitPkt
};
#define RFB_MSG_HEAD "\0"
#define TIMEWAIT_MSG_HEAD "\1"
#define TIMESYNC_MSG_HEAD "\2"
#define FRAMEWAIT_MSG "\3"
#define DDELAY_MSG "\4"
#define CHECKPOINT_MSG "\5"
#define EXIT_MSG "\6"

/* main.cpp */
void error(AU_BOOL perror_en, const char* format, ...);
void hexdump(unsigned char *p, unsigned int len);
void SetNonBlocking(int sock);
AU_BOOL ReadSocket(uint32_t sockfd, unsigned char *ptr, uint32_t len);
AU_BOOL WriteSocket(uint32_t sockfd, unsigned char *ptr, uint32_t len);
AU_BOOL SocketConnected(uint32_t sockfd);


/* server.cpp */
AU_BOOL InitToServer(struct hostent *server, uint32_t portno, uint32_t sockfd);
void InitSI(uint32_t sockfd);
void PrintPixelFormat(PixelFormat *format);
void SetFormatAndEncodings();
void *STCMainLoop(void *sockset);
void WriteServerBuf();
AU_BOOL HandleSTCMsg(SocketSet *s_sockset);
void UpdatePixelData(RectUpdate rect, uint8_t BytesPerPixel);

/* client.cpp */
AU_BOOL ListenTcpPort(uint32_t au_port, uint32_t SockListen);
uint32_t AcceptVncClient(uint32_t SockListen);
AU_BOOL InitToClient(uint32_t sockfd);
void *CTSMainLoop(void *sockset);
void WriteClientBuf();
AU_BOOL ChechClientBuf(uint32_t used);
AU_BOOL HandleCTSMsg(SocketSet *c_sockset);
AU_BOOL HandleSpecialKeys(uint32_t cur_key, AU_BOOL CtrlPressed,
                          AU_BOOL cur_key_state);
void CaptureFrame();
void WriteThreshold();
void InsertFrameWaitPkt();
void InsertTimeWaitPkt();
void InsertSyncPkt();
uint32_t FrameCompare();
void InitThreshold();
void HandleReplayerInput(SocketSet *c_sockset);
AU_BOOL TryMatchFrame();
AU_BOOL SendReplayerInput(SocketSet *c_sockset);
void HandleRfbPkt(SocketSet *c_sockset);
AU_BOOL TryMatchFrame();
AU_BOOL PeekSpecialClick(SocketSet *sockset, uint32_t *peek_result,
                         uint16_t pointer_x, uint16_t pointer_y);
