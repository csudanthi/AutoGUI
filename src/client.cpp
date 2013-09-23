#include "main.h"

/* write back client buffer */
void WriteClientBuf()
{
    ClientToAU.write((const char*)client_buf, (uint32_t)(cbuf_ptr - client_buf));
    memset(client_buf, 0, CLIENT_BUF_SZ);
    cbuf_ptr = client_buf;
}

void CheckClientBuf(uint32_t used)
{
    if(used > CLIENT_BUF_SZ - 100){
        WriteClientBuf();
    }
}

/* return False if socket closed by vnc-client, otherwise return True */
AU_BOOL HandleCTSMsg(SocketSet *c_sockset)
{
    uint32_t c_retnum, encoding_len;
    uint8_t c_msg_type;
    uint16_t encoding_num;
    
    c_retnum = recv(c_sockset->SocketToClient, cbuf_ptr, 1, 0);
    if(c_retnum == 0){  // socket has been closed by vnc-client
        return False;
    }
    else if(c_retnum != 1){
        error(True, "ERROR: [HandleCTSMsg] recv msg type: ");
    }
    c_msg_type = *((uint8_t *)cbuf_ptr);
    cbuf_ptr++;
    //AutoGUI will only forward the framebufferupdaterequest, pointerevent and keyevent message
    switch(c_msg_type){
        case rfbSetPixelFormat: 
            CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
            ReadSocket(c_sockset->SocketToServer, cbuf_ptr, 19);
            cbuf_ptr += 19;
            break;
        case rfbSetEncodings:
            ReadSocket(c_sockset->SocketToServer, cbuf_ptr, 3);
            cbuf_ptr++;
            encoding_num = Swap16(*((uint16_t *)cbuf_ptr));
            encoding_len = 4*encoding_num;
            CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) + encoding_len );
            cbuf_ptr += 2;
            ReadSocket(c_sockset->SocketToServer, cbuf_ptr, encoding_len);
            cbuf_ptr += encoding_len;
            break;
        case rfbFramebufferUpdateRequest:
            CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
            ReadSocket(c_sockset->SocketToServer, cbuf_ptr, 9);
            cbuf_ptr--;
            WriteSocket(c_sockset->SocketToServer, cbuf_ptr, 10);
            cbuf_ptr += 10;
            break;
        case rfbKeyEvent:
            CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
            ReadSocket(c_sockset->SocketToServer, cbuf_ptr, 7);
            cbuf_ptr--;
            WriteSocket(c_sockset->SocketToServer, cbuf_ptr, 8);
            cbuf_ptr += 8;
            break;
           //<== here 
            
            
    }
    

    return True;
}
/* The loop which forward data from Client To Server 
 * return True while user quit, otherwise return False
 */

void *CTSMainLoop(void *sockset)
{
    uint32_t n, i;
    fd_set c_rfds;
    struct timeval tv;
    SocketSet *c_sockset = (SocketSet *)sockset;

    //write back buffer before MainLoop
    WriteClientBuf();

    // Enter Client-To-Server Main Loop
    #ifdef DEBUG
    cout << "Enter Client-To-Server Main Loop" << endl;
    #endif
    while(true){
        FD_ZERO(&c_rfds);
        FD_SET(c_sockset->SocketToClient, &c_rfds);
        tv.tv_sec = 30; //default timeout 30s
        tv.tv_usec = 0;
        n = c_sockset->SocketToClient + 1;
        i = select(n, &c_rfds, NULL, NULL, &tv);
        switch(i){
            case -1:
                error(True, "ERROR: [CTSMainLoop] select ");
            case 0:
                //timeout
                continue;
            case 1:
                //socket can be recv now
                break;
            default:
                error(False, "ERROR: [CTSMainLoop] Should never be here.");
        }
        if( !FD_ISSET(c_sockset->SocketToClient, &c_rfds) ){  //check again
            continue;
        }
        // HandleCTSMsg will return False if socket closed by vnc-client, otherwise return True
        if( !HandleCTSMsg(c_sockset) ){
            return (void *)True;
        }
    }
}


/* AutoGUI will listenning on the specified tcp port */
AU_BOOL ListenTcpPort(uint32_t port, uint32_t SockListen)
{
    uint32_t one = 1;
    struct sockaddr_in addr;

    if( setsockopt(SockListen, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0 ){
        close(SockListen);
        error(True, "ERROR: ListenTcpPort : setsockopt :");
    }
   
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
 
    if( bind(SockListen, (struct sockaddr *)&addr, sizeof(addr)) < 0 ){
        close(SockListen);
        error(True, "ERROR: ListenTcpPort : bind :");
    }

    if( listen(SockListen, 5) < 0 ){
        close(SockListen);
        error(True, "ERROR: ListenTcpPort : listen :");
    }

    return True;
}

/* Waiting for connection request from vnc-client */
uint32_t AcceptVncClient(uint32_t SockListen)
{
    uint32_t sock;
    struct sockaddr_in addr;
    uint32_t addrlen = sizeof(addr);
    
    sock = accept(SockListen, (struct sockaddr *)&addr, &addrlen);
    if(sock < 0){
        error(True, "ERROR: AcceptVncClient: accept:");
    }

    return sock;
}

AU_BOOL InitToClient(uint32_t sockfd)
{
    char RfbProtoVersion[RFBPROTOVER_SZ + 1];
    uint32_t client_major, client_minor;

    /* send RFB proto version used by AutoGUI */
    if( send(sockfd, AU_USED_VER_MSG, RFBPROTOVER_SZ, 0) != RFBPROTOVER_SZ ){
        error(True, "ERROR: cannot send version massage");
    }
   
    /* parse the version msg received from client */
    retnum = recv(sockfd, cbuf_ptr, RFBPROTOVER_SZ, 0);
    memcpy(RfbProtoVersion, cbuf_ptr, RFBPROTOVER_SZ);
    cbuf_ptr += retnum;
    if( sscanf(RfbProtoVersion, "RFB %03d.%03d\n", &client_major, &client_minor) != 2 ){
        error(False, "ERROR: not a valid vnc server");
    }
    if(client_major != MajorVersion || client_minor != MinorVersion){
        error(False, "ERROR: sorry, AutoGUI only support RFB v3.7 right now");
    }

    /* send security type message */
    if( send(sockfd, AU_SEC_TYPE_MSG, 2, 0) != 2 ){
        error(True, "ERROR: cannot send security type massage");
    }
    
    /* read security type */
    retnum = recv(sockfd, cbuf_ptr, 1, 0);
    cbuf_ptr += retnum;

    /* read clientinit message */
    retnum =recv(sockfd, cbuf_ptr, 1, 0);
    cbuf_ptr += retnum;
    #ifdef DEBUG
    hexdump(client_buf, (uint32_t)(cbuf_ptr - client_buf));
    #endif 
 
    /* send serverinit message */
    p_si[SZ_SERVER_INIT_MSG - 1] = 7;
    desktopName = ConstName;
    if( send(sockfd, p_si, SZ_SERVER_INIT_MSG, 0) != SZ_SERVER_INIT_MSG ){
        error(True, "ERROR: cannot send serverinit massage");
    }
    if( send(sockfd, desktopName, 7, 0) != 7 ){
        error(True, "ERROR: cannot send desktopName massage");
    }

    cout << "Successfully connected by vnc client" << endl;
    return True;
}
