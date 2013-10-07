#include "main.h"

/* write back server buffer */
void WriteServerBuf()
{
    ServerToAU.write((const char *)server_buf, (uint32_t)(sbuf_ptr - server_buf));
    memset(server_buf, 0, SERVER_BUF_SZ);
    sbuf_ptr = server_buf;
}

/* return True if writeback, otherwise return False */
AU_BOOL CheckServerBuf(uint32_t used)
{
    if(used > SERVER_BUF_SZ - 200){
        WriteServerBuf();
        return True;
    }
    return False;
}

unsigned char checkbuf[8*1024*1024];
/* return False if socketset are closed by vnc-server or vnc-client, otherwise return True */
AU_BOOL HandleSTCMsg(SocketSet * s_sockset)
{
    uint32_t s_retnum, text_len;
    uint8_t stc_msg_type;
    uint16_t rect_num, i, colour_cnt, colour_len;
    uint32_t rect_size;
    RectUpdate rect;    

    //read stc_msg_type
    s_retnum = recv(s_sockset->SocketToServer, sbuf_ptr, 1, 0);
    if(s_retnum == 0){
        return False;
    }
    else if(s_retnum != 1){
        error(True, "ERROR: [HandleSTCMsg] recv msg type ");
    }
    stc_msg_type = *((uint8_t *)sbuf_ptr);

    #ifdef DEBUG
    cout << "msg_type:";
    hexdump(sbuf_ptr, 1);
    #endif

    // AutoGUI will record all the data received from vnc-server and will forward the necessary messages 
    switch(stc_msg_type){
        case rfbFramebufferUpdate:
            //read and forward framebufferupdate head
            ReadSocket(s_sockset->SocketToServer, sbuf_ptr + 1, HSZ_FRAME_BUFFER_UPDATE - 1);
            rect_num = Swap16(*((uint16_t *)(sbuf_ptr + 2)));
            #ifdef ANALYZE_PKTS
            cout << "#analyze packages# S-->C:rfbFramebufferUpdate           [Forward]" << endl;
            #endif
            #ifdef DEBUG
            cout << "[STCMsg]: " << dec << (int)stc_msg_type << endl;
            cout << "[STCMsg] rect_num: " << dec << (int)rect_num << endl;
            #endif
            if( !WriteSocket(s_sockset->SocketToClient, sbuf_ptr, HSZ_FRAME_BUFFER_UPDATE) ){
                return False;
            }
            sbuf_ptr += HSZ_FRAME_BUFFER_UPDATE;

            for(i = 0; i < rect_num; i++){
                //read and forward rect_update head
                ReadSocket(s_sockset->SocketToServer, sbuf_ptr, SZ_RECT_UPDATE);
                memset(&rect, 0, SZ_RECT_UPDATE);
                memcpy(&rect, sbuf_ptr, SZ_RECT_UPDATE);
                if( !WriteSocket(s_sockset->SocketToClient, sbuf_ptr, SZ_RECT_UPDATE)){
                    return False;
                }
                sbuf_ptr += SZ_RECT_UPDATE;

                rect.x_pos = Swap16(rect.x_pos);
                rect.y_pos = Swap16(rect.y_pos);
                rect.width = Swap16(rect.width);
                rect.height = Swap16(rect.height);
                if( (rect.x_pos + rect.width > si.framebufferWidth) || (rect.y_pos + rect.height > si.framebufferHeight) ){
                    cout << "Rect too large: " << dec << (int)rect.width << "x" << (int)rect.height << "at (" << (int)rect.x_pos << ", " << (int)rect.y_pos << ")" << endl;
                }
                #ifdef ANALYZE_PKTS
                //cout << "si.format.bitsPerPixel[used]: " << dec << (int)si.format.bitsPerPixel << endl;
                #endif
                rect.encoding_type = Swap32(rect.encoding_type);

                /******************************************************************
                 *  AutoGUI won't forward the setencoding message to vnc-server,  *
                 *  and as the RFB Proto Document says:                           *
                 *         "RFB servers should only produce raw encoding,         *
                 *          unless the client asks for other encoding types."     *
                 *  So, AutoGUI only need to handle raw encoding Pixel data.      *
                 ******************************************************************/
                if((int32_t)rect.encoding_type != rfbEncodingRaw){
                    error(False, "ERROR: AutoGUI only support rfbEncodingRaw ");
                }
                rect_size = rect.width*rect.height*(si.format.bitsPerPixel/8);
                #ifdef DEBUG
                cout << "     [STCMsg] rect_size: " << dec << (int)rect_size << endl;
                #endif
                CheckServerBuf( (uint32_t)(sbuf_ptr - server_buf) + rect_size );
              
                // read and forward the rect pixel data
                ReadSocket(s_sockset->SocketToServer, sbuf_ptr, rect_size);
                if( !WriteSocket(s_sockset->SocketToClient, sbuf_ptr, rect_size) ){
                    return False;
                }
                sbuf_ptr += rect_size;
            }
            break;
        case rfbSetColourMapEntries: 
            #ifdef ANALYZE_PKTS
            cout << "#analyze packages# S-->C:rfbSetColourMapEntries           [Forward]" << endl;
            if(si.format.trueColour){
                perror("The vnc-server should use trueColour");
            }
            #endif
            //read and forward setcolourmapentries msg head
            ReadSocket(s_sockset->SocketToServer, sbuf_ptr + 1, HSZ_SET_COLOUR_MAP_ENTRIES - 1);
            colour_cnt = *((uint16_t *)(sbuf_ptr + 4));
            colour_len = 6*colour_cnt;
            if( !WriteSocket(s_sockset->SocketToClient, sbuf_ptr, HSZ_SET_COLOUR_MAP_ENTRIES) ){
                return False;
            }
            sbuf_ptr += HSZ_SET_COLOUR_MAP_ENTRIES;

            
            CheckServerBuf((uint32_t)(sbuf_ptr - server_buf) + colour_len);
            //read and forward setcolourmapentries msg data
            ReadSocket(s_sockset->SocketToServer, sbuf_ptr, colour_len);
            if( !WriteSocket(s_sockset->SocketToClient, sbuf_ptr, colour_len) ){
                return False;
            }
            sbuf_ptr += colour_len;
            break;
        case rfbBell:
            #ifdef ANALYZE_PKTS
            cout << "#analyze packages# S-->C:rfbBell" << endl;
            #endif
            sbuf_ptr++;
            break;
        case rfbServerCutText:
            #ifdef ANALYZE_PKTS
            cout << "#analyze packages# S-->C:rfbServerCutText                 [Forward]" << endl;
            #endif
            //read and forward cuttext msg head
            ReadSocket(s_sockset->SocketToServer, sbuf_ptr + 1, HSZ_SERVER_CUT_TEXT - 1);
            text_len = Swap32(*((uint32_t *)(sbuf_ptr + 4)));
            if( !WriteSocket(s_sockset->SocketToClient, sbuf_ptr, HSZ_SERVER_CUT_TEXT) ){
                return False;
            }
            sbuf_ptr += HSZ_SERVER_CUT_TEXT;

            CheckServerBuf((uint32_t)(sbuf_ptr - server_buf) + text_len);
            //read and forward cuttext msg data
            ReadSocket(s_sockset->SocketToServer, sbuf_ptr, text_len);
            if( !WriteSocket(s_sockset->SocketToClient, sbuf_ptr, text_len) ){
                return False;
            }
            sbuf_ptr += text_len;
            break;
        default:
            error(False, "ERROR: [HandleSTCMsg] This msg-type(%d) should not appear.", stc_msg_type);
    }
    #ifdef DEBUG
    cout << "the data in server_buf:" << endl;
    hexdump(server_buf, (unsigned int)(sbuf_ptr - server_buf));
    #endif
    return True;
}

/**********************************************************
 *   The loop which forward data from Server To Client    *
 *   return True if user quit, otherwise keep going       * 
 **********************************************************/
void *STCMainLoop(void *sockset)
{
    uint32_t n, i;
    fd_set s_rfds;
    struct timeval tv;
    SocketSet *s_sockset = (SocketSet *)sockset;

    //write back buffer before Main Loop
    WriteServerBuf();

    // Enter Server-To-Client Main Loop
    #ifdef DEBUG
    cout << "Enter Server-To-Client Main Loop" << endl;
    #endif
    while(true){
        //Check the vnc-client is connect or not
        if(!SocketConnected(s_sockset->SocketToClient)){
            // The vnc-client has closed, AutoGUI will exit also.
            return (void *)True;
        }
        FD_ZERO(&s_rfds);
        FD_SET(s_sockset->SocketToServer, &s_rfds);
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        n = s_sockset->SocketToServer + 1;
        i = select(n, &s_rfds, NULL, NULL, &tv);
        switch(i){
            case -1:
                perror("ERROR: [STCMainLoop] select ");
            case 0:
                //timeout
                continue;
            case 1:
                //socket can be read now
                break;
            default:
                error(False, "ERROR: [STCMainLoop] select: should not be here.");
        }
        if( !FD_ISSET(s_sockset->SocketToServer, &s_rfds) ){  //check again
            continue;
        }
        
        // HandleSTCMsg will return False if socket is closed by vnc-server, otherwise return True
        if( !HandleSTCMsg(s_sockset) ){
            return (void *)True;
        }
    }
}

/* setup the pixel data format and the encoding of pixel data */
void SetFormatAndEncodings()
{
    /* This function is empty because we wish our record file can be used by 
     * all the vnc-server, including gem5 simulator. 
     * So, AutoGUI won't send/forward any "SetEncodings" messages to vnc-server.
     * 
     * But AutoGUI will forward the "SetPixelFormat" message to vnc-server,
     * because the once the vnc-client set a "SetPixelFormat" message, 
     * it will parse the pixel data by the format specified in the "SetPixelFormat" message.
     */
}

/* initialize the si by the server-init message*/
void InitSI(uint32_t sockfd)
{
    const AU_BOOL swap = True; //the server-init is little-endian , but the host byte order is big-endian, so swap

    memset(&si, 0, SZ_SERVER_INIT_MSG);
    memcpy(&si, sbuf_ptr, SZ_SERVER_INIT_MSG);
    sbuf_ptr += SZ_SERVER_INIT_MSG;
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
    cout << "the data in server_buf:" << endl;
    hexdump(server_buf, (unsigned int)(sbuf_ptr - server_buf));
    #endif
}

/* print the default server pixelformat */
void PrintPixelFormat(PixelFormat *format)
{
    cout << "The default server PixelFormat:" << endl \
         << (unsigned int)format->bitsPerPixel << " bits per pixel." << endl \
         << (format->bigEndian ? "Most" : "Least") << " significant byte first in each pixel." << endl;
    if(format->trueColour){
        cout << "True colour: max red: " << (unsigned int)format->redMax << " green: " << (unsigned int)format->greenMax \
             << " blue: " << (unsigned int)format->blueMax << ", shift red: " << (unsigned int)format->redShift \
             << " green: " << (unsigned int)format->greenShift << " blue: " << (unsigned int)format->blueShift << endl;
    }
    else{
        cout << "Colour map (not true colour)." << endl;
    }
    cout << "The width and height of framebuffer is: " << dec << (int)si.framebufferWidth << "," << (int)si.framebufferHeight << endl;
    cout << endl;
}


/* finish the connection between AutoGUI and vnc-server */
AU_BOOL InitToServer(struct hostent *server, uint32_t portno, uint32_t sockfd)
{
    struct sockaddr_in serv_addr;
    char RfbProtoVersion[RFBPROTOVER_SZ + 1];

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
    cout << "The RFB proto version of server is:" << endl << RfbProtoVersion << endl;
    #endif
/*
*/
    if( sscanf(RfbProtoVersion, "RFB %03d.%03d\n", &server_major, &server_minor) != 2 ){
        error(False, "ERROR: not a valid vnc server");
    }
    if(server_major != MajorVersion || server_minor < MinorVersion){
        error(False, "ERROR: sorry, AutoGUI only support RFB v3.7 and v3.8 right now");
    }

    /* send RFB proto version used by AutoGUI */
    if(server_minor == 8){
        if( send(sockfd, AU_USED_VER_MSG_38, RFBPROTOVER_SZ, 0) != RFBPROTOVER_SZ ){
            error(True, "ERROR: cannot send version massage");
        }
        cout << "The RFB version used by AutoGUI is :" << AU_USED_VER_MSG_38 <<endl;
    }
    else if(server_minor == 7){
        if( send(sockfd, AU_USED_VER_MSG_37, RFBPROTOVER_SZ, 0) != RFBPROTOVER_SZ ){
            error(True, "ERROR: cannot send version massage");
        }
        cout << "The RFB version used by AutoGUI is :" << AU_USED_VER_MSG_37 <<endl;
    }
    else{
        error(False, "ERROR: server_minor should be 7 or 8");
    }
    #ifdef DEBUG
    cout << "The RFB version used by AutoGUI is :" << AU_USED_VER_MSG <<endl;
    #endif
    
    /* read security type message */
    retnum = recv(sockfd, sbuf_ptr, 1, 0);
    next_len = *(uint32_t *)(sbuf_ptr);
    sbuf_ptr += retnum;
    retnum = recv(sockfd, sbuf_ptr, next_len, 0);
    if(*(uint32_t *)sbuf_ptr == 0){
        error(False, "ERROR: there is not security-type provided by server");
    }
    sbuf_ptr += retnum;

    /* send security type */
    if(send(sockfd, (char *)&AU_SEC_TYPE, 1, 0) != 1){
        error(True, "ERROR: cannot send security type");
    }

    //only the v3.8 will return 4 bytes check info
    if(server_minor == 8){
        retnum = recv(sockfd, sbuf_ptr, 4, 0);
        if(retnum != 4){
            error(True, "ERROR: cannot get the security type check info");
        }
        if( *((uint32_t *)sbuf_ptr) != 0 ){
            error(False, "ERROR: cannot use none security type");
        }
        #ifdef DEBUG
        cout << "the check info received from vnc-server:" << endl;
        hexdump((unsigned char *)sbuf_ptr, 4);
        #endif
        sbuf_ptr += retnum;
    }

    
    /* send clientinit message */
    if(send(sockfd, (char *)&AU_SHARED_FLAG, 1, 0) != 1){
        error(True, "ERROR: cannot send shared-flag");
    }
    
    /* read serverinit message and store a copy*/
    retnum = recv(sockfd, sbuf_ptr, SZ_SERVER_INIT_MSG, 0);
    if(retnum != SZ_SERVER_INIT_MSG){
        error(True, "ERROR: cannot received server initialization message");
    }
    p_si = (unsigned char *)malloc(SZ_SERVER_INIT_MSG);
    if(!p_si){
        error(True, "ERROR: cannot malloc memory for p_si");
    }
    memset(p_si, 0, SZ_SERVER_INIT_MSG);
    memcpy(p_si, sbuf_ptr, SZ_SERVER_INIT_MSG);

    /* initialize si */
    InitSI(sockfd);
    
    #ifdef DEBUG
    PrintPixelFormat(&si.format);
    #endif

    /* setup the pixel data format and the encoding of pixel data */
    SetFormatAndEncodings();

    cout << "[Step one] Successfully connect to vnc server" << endl << endl;
    return True; 
}

