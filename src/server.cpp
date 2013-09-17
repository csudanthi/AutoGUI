#include "main.h"

void InitSI(uint32_t sockfd)
{
    AU_BOOL swap;
    if( *(uint8_t *)(sbuf_ptr+6) == 0){
        swap = True;	//the pixel value is little-endian, but the host byte order is big-endian, so swap
    }
    else{
        swap = False;
    }

    memset(&si, 0, SZ_SERVER_INIT_MSG);
    si.framebufferWidth = *(uint16_t *)sbuf_ptr;


    sbuf_ptr += 2;
    si.framebufferHeight = *(uint16_t *)sbuf_ptr;
    sbuf_ptr += 2;

    si.format.bitsPerPixel = *(uint8_t *)sbuf_ptr;
    sbuf_ptr++;
    si.format.depth = *(uint8_t *)sbuf_ptr;
    sbuf_ptr++;
    si.format.bigEndian = *(uint8_t *)sbuf_ptr;
    sbuf_ptr++;
    si.format.trueColour = *(uint8_t *)sbuf_ptr;

    sbuf_ptr++;
    si.format.redMax = *(uint16_t *)sbuf_ptr;
    sbuf_ptr += 2;
    si.format.greenMax = *(uint16_t *)sbuf_ptr;
    sbuf_ptr += 2;
    si.format.blueMax = *(uint16_t *)sbuf_ptr;
    sbuf_ptr += 2;
    si.format.redShift = *(uint8_t *)sbuf_ptr;
    sbuf_ptr++;
    si.format.greenShift = *(uint8_t *)sbuf_ptr;
    sbuf_ptr++;
    si.format.blueShift = *(uint8_t *)sbuf_ptr;
    sbuf_ptr += 4;

    si.nameLength = *(uint32_t *)sbuf_ptr;
    sbuf_ptr += 4;

    if(swap){
        si.framebufferWidth = Swap16(si.framebufferWidth);
        si.framebufferHeight = Swap16(si.framebufferHeight);
        si.format.redMax = Swap16(si.format.redMax);
        si.format.greenMax = Swap16(si.format.greenMax);
        si.format.blueMax = Swap16(si.format.blueMax);
        si.nameLength = Swap32(si.nameLength);
    }

    #ifdef DEBUG
    cout << "bigEndian:" << dec << (unsigned int)si.format.bigEndian  << endl;
    cout << "nameLength:" << dec << (unsigned int)si.nameLength  << endl;
    #endif
    //In AutoGUI, we will replace desktopName by string "AutoGUI" defaultly. --zhongbin
    desktopName = (unsigned char *)malloc(si.nameLength + 1);
    if(!desktopName){
        error(True, "ERROR: cannot malloc memory for desktopName");
    }
    retnum = recv(sockfd, sbuf_ptr, si.nameLength, 0);
    if(retnum != si.nameLength){
        error(True, "ERROR: cannot receive desktopName");
    }
    memcpy(desktopName, sbuf_ptr, si.nameLength);
    desktopName[si.nameLength] = '\0';
    sbuf_ptr += retnum;
    #ifdef DEBUG
    cout << "desktopName:" << desktopName << endl;
    cout << "retnum: " << dec << retnum << "     si.nameLength:" << si.nameLength << endl;
    hexdump(server_buf, (unsigned int)(sbuf_ptr - server_buf));
    #endif
}

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
/*
    #ifdef DEBUG
    hexdump((unsigned char *)server_buf, (uint32_t)(sbuf_ptr - server_buf));
    cout << "The RFB proto version of server is:" << endl << RfbProtoVersion << endl;
    #endif
*/
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
    
    /* read security type message */
    retnum = recv(sockfd, sbuf_ptr, 1, 0);
    next_len = *(uint32_t *)(sbuf_ptr);
    sbuf_ptr += retnum;
    retnum = recv(sockfd, sbuf_ptr, next_len, 0);
    if(*(uint32_t *)sbuf_ptr == 0){
        error(False, "ERROR: there is not security-type provided by server");
    }
    sbuf_ptr += retnum;
    #ifdef DEBUG
    hexdump((unsigned char *)server_buf, (uint32_t)(sbuf_ptr - server_buf));
    #endif
    
    /* send security type and read check info */
    if(send(sockfd, (char *)&AU_SEC_TYPE, 1, 0) != 1){
        error(True, "ERROR: cannot send security type");
    }
    retnum = recv(sockfd, sbuf_ptr, 4, 0);
    if(retnum != 4){
        error(True, "ERROR: cannot get the security type check info");
    }
    sbuf_ptr += retnum;
    #ifdef DEBUG
    hexdump((unsigned char *)server_buf, (uint32_t)(sbuf_ptr - server_buf));
    #endif
    
    /* send clientinit message */
    if(send(sockfd, (char *)&AU_SHARED_FLAG, 1, 0) != 1){
        error(True, "ERROR: cannot send shared-flag");
    }
    
    /* read serverinit message */
    //retnum = recv(sockfd, sbuf_ptr, SZ_SERVER_INIT_MSG, 0);
    retnum = recv(sockfd, sbuf_ptr, 24, 0);
    //if(retnum != SZ_SERVER_INIT_MSG){
    if(retnum != 24){
        error(True, "ERROR: cannot received server initialization message");
    }

    /* initialize si */
    InitSI(sockfd);

    cout << "happy" << endl;
   
    return True; 
}

