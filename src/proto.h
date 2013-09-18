
/* boolean type for AutoGUI */
typedef int AU_BOOL;
#ifndef True
#define False (0)
#define True (1)
#endif

/* Macros for endian swapping. */
#define Swap16(s) ( (((s) & 0xff) << 8) | (((s) >> 8) & 0xff) )
#define Swap32(s) ( ((s) >> 24) | \
                    ((s) & 0x00ff0000) >> 8 | \
                    ((s) & 0x0000ff00) << 8 | \
                    ((s) << 24) \
                  )

/* Structure used to specify pixel format */
typedef struct _PixelFormat{
    uint8_t bitsPerPixel;	/*8,16,32 only*/
    uint8_t depth;		/*8 to 32*/
    uint8_t bigEndian;		/*True or False*/
    uint8_t trueColour;		/*True or False*/
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
typedef struct _ServerInitMsg{
    uint16_t framebufferWidth;
    uint16_t framebufferHeight;
    PixelFormat format;	/* the server preferredd pixel format */
    uint32_t nameLength;
    /* followed by char name[nameLength] */
}ServerInitMsg;
#define SZ_SERVER_INIT_MSG (8 + SZ_PIXEL_FORMAT)



/* main.cpp */
void error(AU_BOOL perror_en, const char* format, ...);
void hexdump(unsigned char *p, unsigned int len);
void SetNonBlocking(int sock);


/* server.cpp */
AU_BOOL InitToServer(struct hostent *server, uint32_t portno, uint32_t sockfd);
void InitSI(uint32_t sockfd);
void PrintPixelFormat(PixelFormat *format);
void SetFormatAndEncodings();
