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

#include "main.h"

// file to write client input data
ofstream ClientToAU;
// file to write server output data
ofstream ServerToAU;
// buffer to hold server output data
unsigned char server_buf[SERVER_BUF_SZ];
// buffer to hold client input data
unsigned char client_buf[CLIENT_BUF_SZ];
unsigned char *sbuf_ptr = server_buf;
unsigned char *cbuf_ptr = client_buf;
// bytes read from socket
uint32_t retnum = 0;
// the length will be received next, usually specified by the head-package
uint32_t next_len = 0;
struct timeval FrameBegin, FrameEnd;

// the mask state of pointerevent.
uint8_t button_mask = 0;
AU_BOOL CtrlPressed = False;
AU_BOOL Recording = False;
AU_BOOL Replaying = False;
uint32_t EventCnt;
uint32_t FrameCnt;
// current pixel data in default rectangle size
uint32_t CurPixelData[RECT_CY+20][RECT_CX+20];
// captured pixel data in default rectangle size
uint32_t CapPixelData[RECT_CY+20][RECT_CX+20];
// update flag for CurPixelData
AU_BOOL UpdateFlag = False;

// the max version of RFB supported by vnc-server
uint32_t server_major, server_minor;

ServerInitMsg si;
unsigned char ConstName[] = "AutoGUI";
unsigned char *desktopName = ConstName;
unsigned char *p_si = NULL;

// default secure type(no authentication) for AutoGUI
uint8_t AU_SEC_TYPE = 1;
// default shared-flag(exclusive) for AutoGUI
uint8_t AU_SHARED_FLAG = 0;
// label for the first input while recording/replaying
AU_BOOL FirstInput = False;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// file to write log info for debug
ofstream log;
// file to write ultimate recorded data for replayer
ofstream RecordFile;
// file to read ultimate recorded data for replayer
ifstream ReplayFile;
// file to write threshold for each pixel data
ofstream PixelThres;
// file to read  threshold for each pixel data
ifstream PixelThresR;
// file to write pixel data in every ractangle frame
ofstream RectFramePixel;
// file to read  pixel data in every ractangle frame
char pathRectFramePixel[1024];
ifstream RectFramePixelR;
float *thres_list = NULL;
uint32_t thres_list_len;
// global threshold used by function:TryMatchFrame
float RfbFrameThres = 0.0;


AU_BOOL ReadSocket(uint32_t sockfd, unsigned char *ptr, uint32_t len)
{
    uint32_t retnum, lentoread;
    unsigned char *buf_ptr = ptr;
    if (!SocketConnected(sockfd)) {
        return False;
    }
    while (len > 0) {
        lentoread = len < SZ_PER_OPT ? len:SZ_PER_OPT;
        retnum = recv(sockfd, buf_ptr, lentoread, 0);
        if (retnum <= 0) {
            error(True, "ERROR: [socket id:%d] recv: ", sockfd);
        }
        buf_ptr += retnum;
        len -= retnum;
    }
    if (len != 0) {
        error(False, "ERROR: [ReadSocket:%d] read too much", sockfd);
    }
    return True;
}

AU_BOOL WriteSocket(uint32_t sockfd, unsigned char *ptr, uint32_t len)
{
    uint32_t retnum, lentowrite;
    unsigned char *buf_ptr = ptr;
    if (!SocketConnected(sockfd)) {
        return False;
    }
    while (len > 0) {
        lentowrite = len < SZ_PER_OPT ? len:SZ_PER_OPT;
        retnum = send(sockfd, buf_ptr, lentowrite, 0);
        if (retnum <= 0) {
            error(True, "ERROR: [socket id:%d] send: ", sockfd);
        }
        len -= retnum;
        buf_ptr += retnum;
    }
    if (len != 0) {
        error(False, "ERROR: [WriteSoocket:%d] write too much", sockfd);
    }
    return True;
}

/* handle the errors */
void error(AU_BOOL perror_en, const char* format, ...) {
    va_list args;
    char error_msg[1024];
    va_start(args, format);
    vsprintf(error_msg, format, args);
    va_end(args);

    if (perror_en) {
        perror(error_msg);
    } else {
        cerr << error_msg << endl;
    }
    #ifdef DEBUG
    ServerToAU.write((const char *)server_buf,
                     (uint32_t)(sbuf_ptr - server_buf));
    ClientToAU.write((const char*)client_buf,
                     (uint32_t)(cbuf_ptr - client_buf));
    #endif
    exit(0);
}

/* produce a hex dump */
void hexdump(unsigned char *p, uint32_t len)
{
    unsigned char *line = p;
    uint32_t i, linelen, offset = 0;

    while (offset < len) {
        cout << hex << setfill('0') << setw(4) << offset << ' ';

        linelen = len - offset;
        linelen = linelen > 16 ? 16:linelen;

        for (i = 0; i < linelen; i++) {
            cout << hex << setfill('0') << setw(2) << (int)line[i] << ' ';
        }
        for (; i < 16; i++) {
            cout << "   ";
        }

        for (i = 0; i < linelen; i++) {
            unsigned char alpha_char;
            alpha_char = (line[i] >= 0x20 && line[i] < 0x7f) ? line[i]:'.';
            cout << alpha_char;
        }
        cout << endl;

        offset += linelen;
        line += linelen;
    }
    cout << endl;
}

/* set a given socket into non-blocking mode
 * not used yet --zhongbin */
void SetNonBlocking(int sock)
{
    int flags;
    if ((flags = fcntl(sock, F_GETFL, 0)) < 0) {
        flags = 0;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        error(True, "ERROR: cannot set socket into non-blocking");
    }
}

/* check a socket is closed or not */
AU_BOOL SocketConnected(uint32_t sockfd)
{
    struct tcp_info info;
    int len = sizeof(info);

    if (sockfd <= 0) {
        return False;
    }

    getsockopt(sockfd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
    if ( info.tcpi_state == TCP_ESTABLISHED ) {
        return True;
    }
    #ifdef DEBUG
    else {
        cout << "Sockfd:" << dec << (int)sockfd << " tcpi_state = "
             << (int)info.tcpi_state << endl;
    }
    #endif
    return False;
}


int main(int argc, char *argv[])
{
    uint32_t SockToServer, SockToClient, SockListen, server_port, au_port;
    struct hostent *server;

    // parse argv, set server-name, server-port and AutoGUI port
    if (argc < 4) {
        error(False, "ERROR: invalid arguments.  Usage is '%s <hostname>"
                     " <server-port> <AutoGUI-port>'", argv[0]);
    }
    server_port = atoi(argv[2]);
    if (server_port < VNC_PORT_BASE) {
        error(False, "ERROR: invalid port number.  Port number should not"
                     " less then 5900");
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        error(False, "ERROR: cannot find host");
    }
    au_port = atoi(argv[3]);
    if (au_port < VNC_PORT_BASE) {
        error(False, "ERROR: invalid listenning port number.  Port number"
                     " should not less then 5900");
    }

    // create sockets
    SockToServer = socket(AF_INET, SOCK_STREAM, 0);
    if (SockToServer < 0) {
        error(True, "ERROR: cannot open socket");
    }
    SockListen = socket(AF_INET, SOCK_STREAM, 0);
    if (SockListen < 0) {
        error(True, "ERROR: cannot open socket");
    }

    if ( mkdir("./framein", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0
         && errno != EEXIST) {
        error(True, "can not create base_path folder(framein)");
    }
    ClientToAU.open("./framein/UsrIn.pkts");
    ServerToAU.open("./framein/VncOut.pkts");
    log.open("./framein/log.txt");
    if ( !ClientToAU.is_open() || !ServerToAU.is_open() || !log.is_open() ) {
        error(True, "Cannot open framein files");
    }

    // Initialize the connection between AutoGUI and VNC-Server,
    // include handshakes
    if ( !InitToServer(server, server_port, SockToServer) ) {
        error(False, "ERROR: cannot complete the init to vnc-server");
    }

    // Listen on the au_port which the vnc-client will connect to
    if ( !ListenTcpPort(au_port, SockListen) ) {
        error(False, "ERROR: cannot listen on port(%d)", au_port);
    }

    // Wait for vnc-client
    SockToClient = AcceptVncClient(SockListen);
    #ifdef DEBUG
    cout << "A VNC client has been accepted." << endl;
    cout << (int)SockToClient << endl;
    #endif

    // Initialize the connection between AutoGUI and VNC-Client,
    // include handshakes
    if ( !InitToClient(SockToClient) ) {
        error(False, "ERROR: cannot complete the init to vnc-client");
    }

    // The main loop of AutoGUI
    SocketSet sockset;
    sockset.SocketToClient = SockToClient;
    sockset.SocketToServer = SockToServer;
    #ifdef DEBUG
    cout << "SocketToServer: " << SockToServer << endl;
    cout << "SocketToClient: " << SockToClient << endl;
    #endif

    pthread_t ctid, stid;
    void *c_return, *s_return;

    if ( pthread_create(&ctid, NULL, CTSMainLoop, (void *)&sockset) != 0) {
        error(True, "ERROR: pthread_create ctid(%d): ", ctid);
    }
    if ( pthread_create(&stid, NULL, STCMainLoop, (void *)&sockset) != 0) {
        error(True, "ERROR: pthread_create stid(%d): ", stid);
    }

    // Wait the child-threads exit
    if ( pthread_join(ctid, &c_return) != 0) {
        error(True, "ERROR: pthread_join ctid(%d): ", ctid);
    }
    cout << "[Step three] CTS Main Loop exit" << endl;
    cout << "             Waiting the STC Main Loop exit" << endl;
    if ( pthread_join(stid, &s_return) != 0) {
        error(True, "ERROR: pthread_join stid(%d): ", stid);
    }
    cout << "             STC Main Loop exit" << endl << endl;

    if ( (((AU_BOOL *)c_return)) && (((AU_BOOL *)s_return)) ) {
        log << "All tasks have finished." << endl;
    }


    ServerToAU.write((const char *)server_buf,
                     (uint32_t)(sbuf_ptr - server_buf));
    ClientToAU.write((const char*)client_buf,
                     (uint32_t)(cbuf_ptr - client_buf));
    ClientToAU.close();
    ServerToAU.close();
    log.close();

    return 0;
}
