#include "main.h"

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
