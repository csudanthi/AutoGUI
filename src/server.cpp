#include "main.h"


AU_BOOL InitToServer(struct hostent *server, uint32_t portno, uint32_t sockfd)
{
    struct sockaddr_in serv_addr;
    char RfbProtoVersion[RFBPROTOVER_SZ + 1];
    uint32_t server_major, server_minor;

    /* set serv_addr */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    /* bulid connection */
    if (connect(sockfd, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        error(True, "ERROR: cannot connect to server");
    }

    /* parse the version msg received from server */
    retnum = recv(sockfd, sbuf_ptr, RFBPROTOVER_SZ, 0);
    memcpy(RfbProtoVersion, sbuf_ptr, RFBPROTOVER_SZ);
    sbuf_ptr += retnum;
    #ifdef DEBUG
    hexdump((unsigned char *)server_buf, (uint32_t)(sbuf_ptr - server_buf));
    cout << "The RFB proto version of server is:" << endl << RfbProtoVersion << endl;
    #endif
    if( sscanf(RfbProtoVersion, "RFB %03d.%03d\n", &server_major, &server_minor) != 2 ){
        error(False, "ERROR: not a valid vnc server");
    }
    if(server_major != MajorVersion || server_minor < MinorVersion){
        error(False, "ERROR: sorry, AutoGUI only support RFB3.7 right now");
    }

    /* send RFB proto version used by AutoGUI */
    if( send(sockfd, AU_USED_VER_MSG, RFBPROTOVER_SZ, 0) != RFBPROTOVER_SZ ){
        error(True, "ERROR: cannot send version massage");
    }
    
    //read security type
    retnum = recv(sockfd, sbuf_ptr, 1, 0);
    next_len = *(uint32_t *)(sbuf_ptr);
    cout << "next_len:" << next_len << endl;
    sbuf_ptr += retnum;
    retnum = recv(sockfd, sbuf_ptr, next_len, 0);
    if(retnum != next_len){
        error(False, "ERROR: cannot %d bytes data", next_len);
    }
    sbuf_ptr += retnum;
    #ifdef DEBUG
    hexdump((unsigned char *)server_buf, (uint32_t)(sbuf_ptr - server_buf));
    #endif
    
    //send security type
    //
    
    
    return True;
}
