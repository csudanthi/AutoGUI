#include "main.h"

/* write back client buffer */
void WriteClientBuf()
{
    ClientToAU.write((const char*)client_buf, (uint32_t)(cbuf_ptr - client_buf));
    memset(client_buf, 0, CLIENT_BUF_SZ);
    cbuf_ptr = client_buf;
}

/* return True if writeback, otherwise return False */
AU_BOOL CheckClientBuf(uint32_t used)
{
    if(used > CLIENT_BUF_SZ - 100){
        WriteClientBuf();
        return True;
    }
    return False;
}

/* Initialize threshold list */
void InitThreshold()
{
    uint16_t i, thres_list_len;
	string line;
	thres_list_len = 0;

    PixelThresR.open("./framein/fb.thres");
	if(!PixelThresR.is_open()){
		error(True, "Cannot open PixelThresR to initialize thres_list");
	}
	//compute thres_list_len
	while(getline(PixelThresR, line)){
		thres_list_len ++;
	}
	PixelThresR.close();
    PixelThresR.open("./framein/fb.thres");
	if(!PixelThresR.is_open()){
		error(True, "Cannot open PixelThresR to initialize thres_list");
	}
	if(thres_list != NULL){
		free(thres_list);
		thres_list = NULL;
	}
	thres_list = (float *)malloc(sizeof(float) * thres_list_len);
	if(thres_list == NULL){
		error(True, "Cannot malloc memory for thres_list");
	}
	memset(thres_list, 0, thres_list_len);

	for(i = 0; i < thres_list_len; i++){
		PixelThresR >> thres_list[i];
	}	
	pthread_mutex_lock(&mutex);
	log << "Initialize thres_list. len = " << dec << (int)thres_list_len << endl;
	for(i = 0; i < thres_list_len; i++){
		log << dec << (float)thres_list[i] << endl;
	}
	pthread_mutex_unlock(&mutex);
}

/* return True if special keystrokes appears, otherwise return False */
AU_BOOL HandleSpecialKeys(uint32_t cur_key, AU_BOOL CtrlPressed, AU_BOOL cur_key_state)
{
	static AU_BOOL ingnore_next_release = False;
	if(CtrlPressed && cur_key_state != False){
		switch(cur_key){
			case XK_s:
				if(Replaying){
					cout << "It's replay mode now, should not send any user input" << endl;
					return True;
				}
				//begin to capture input stream
				Recording = !Recording;
				if(Recording){
					EventCnt = 0;
					FrameCnt = 0; 	
					FirstInput = True;
					cout << "####  Start Record  ####" << endl;

					pthread_mutex_lock(&mutex);
					log << "####  Start Record  ####" << endl;
					pthread_mutex_unlock(&mutex);
					RecordFile.open("./framein/icapture_rfb.ibin");
    				PixelThres.open("./framein/fb.thres");
					if(!RecordFile.is_open() || !PixelThres.is_open()){
						error(True, "Cannot open files for record");
					}
				}
				else{
					pthread_mutex_lock(&mutex);
					log << "EventCnt captured: " << dec << (int)EventCnt << endl;
					log << "FrameCnt captured: " << dec << (int)FrameCnt << endl;
					log << "####  End Record    ####" << endl;
					pthread_mutex_unlock(&mutex);
					cout << "####  End Record    ####" << endl;
					cout << "   insert a exotPkt by default  " << endl;
					RecordFile.write(EXIT_MSG, 1);
					RecordFile.close();
    				PixelThres.close();
				}
				ingnore_next_release = True;
				return True;
				break;
			case XK_i:
				if(Recording){
					cout << "It's record mode now, can not changed into replay mode" << endl;
					return True;
				}
				//begin to replay captured input stream
				Replaying = !Replaying;
				if(Replaying){
					EventCnt = 0;
					FrameCnt = 0;
					FirstInput = True;
					cout << "####  Start Replay  ####" << endl;

					pthread_mutex_lock(&mutex);
					log << "####  Start Replay  ####" << endl;
					pthread_mutex_unlock(&mutex);
					ReplayFile.open("./framein/icapture_rfb.ibin");
					if(!ReplayFile.is_open()){
						error(True, "Cannot open RecordFile for replay");
					}
					InitThreshold();
				}
				else{
					pthread_mutex_lock(&mutex);
					log << "EventCnt captured: " << dec << (int)EventCnt << endl;
					log << "FrameCnt captured: " << dec << (int)FrameCnt << endl;
					log << "####  End Replay    ####" << endl;
					pthread_mutex_unlock(&mutex);
					cout << "####  End Replay    ####" << endl;
					ReplayFile.close();
    				PixelThresR.close();
				}
				ingnore_next_release = True;
				return True;
				break;
           	case XK_c:
				//insert checkpoint packet
				cout << "Capture a chkpointPkt" << endl;
				if(Recording){
					pthread_mutex_lock(&mutex);
					log << "Capture a chkpointPkt" << endl;
					pthread_mutex_unlock(&mutex);
					RecordFile.write(CHECKPOINT_MSG, 1);
				}
				ingnore_next_release = True;
				return True;
				break;
			case XK_q:
				//insert exit packet
				cout << "Capture a exitPkt" << endl;
				if(Recording){
					pthread_mutex_lock(&mutex);
					log << "Capture a exitPkt" << endl;
					pthread_mutex_unlock(&mutex);
					RecordFile.write(EXIT_MSG, 1);
				}
				ingnore_next_release = True;
				return True;
				break;
			case XK_d:
				//insert directed delay packet
				cout << "Capture a directed delay packet" << endl;
				if(Recording){
					pthread_mutex_lock(&mutex);
					log << "Capture a directed delay packet" << endl;
					pthread_mutex_unlock(&mutex);
					RecordFile.write(DDELAY_MSG, 1);
				}
				ingnore_next_release = True;
				return True;
				break;
			default:
				return False;
		}
	}
	if(ingnore_next_release == True){		//ignore the special key release
		ingnore_next_release = False;
		return True;
	}
	return False;
}

/* compare CurPixelData and CapPixelData, return the different pixel number */
uint32_t FrameCompare()
{
	uint32_t num_px_diff;
	uint16_t i, j;
	num_px_diff = 0;
	for(i = RECT_X; i < RECT_CX; i++){
		for(j = RECT_Y; j < RECT_CY; j++){
			if(CurPixelData[i][j] != CapPixelData[i][j]){
				num_px_diff++;
			}
		}
	} 
	return num_px_diff;
}

/* capture a frame into CapPixelData then write into RecordFile */
void CaptureFrame()
{
	pthread_mutex_lock(&mutex);
	memcpy(CapPixelData[0], CurPixelData[0], sizeof(CurPixelData));
	log << "[Recording]Capture a frame pixel data(framecnt=" << dec << (int)FrameCnt << ")" << endl;
	pthread_mutex_unlock(&mutex);

	sprintf(pathRectFramePixel, "framein/fb%06u.raw", FrameCnt);
	RectFramePixel.open(pathRectFramePixel);
	if(!RectFramePixel.is_open()){
		error(True, "Can not open a file for frame pixel data");
	}
	RectFramePixel.write((char *)CapPixelData[0], sizeof(CapPixelData));
	RectFramePixel.close();
}

/* write the computed threshold into PixelThres */
void WriteThreshold()
{
	float threshold;
	uint32_t sum_px, num_px_diff;
	sum_px = (RECT_CX - RECT_X) * (RECT_CY - RECT_Y);

	pthread_mutex_lock(&mutex);
	log << "[Recording]Compute threshold for a captured frame(framecnt=" << dec << (int)FrameCnt << ")"  << endl;
	num_px_diff = FrameCompare();
	threshold = 1 - ((num_px_diff + (sum_px - num_px_diff)*0.05)*1.0)/sum_px;
	log << "   threshold = " << setprecision(3) << threshold << endl;
	pthread_mutex_unlock(&mutex);

	PixelThres << setprecision(3) << threshold << endl;
}

/* insert a framewaitPkt */
void InsertFrameWaitPkt()
{
	pthread_mutex_lock(&mutex);
	log << "[Recording]Insert a framewaitPkt(framecnt=" << dec << (int)FrameCnt << ")" << endl;
	pthread_mutex_unlock(&mutex);

	RecordFile.write(FRAMEWAIT_MSG, 1);
}

/* insert a timewaitPkt */
void InsertTimeWaitPkt()
{
	uint32_t WaitTime;	//us
	gettimeofday(&FrameEnd, NULL);
	WaitTime = (FrameEnd.tv_sec - FrameBegin.tv_sec)*1000*1000 + (FrameEnd.tv_usec - FrameBegin.tv_usec);

	pthread_mutex_lock(&mutex);
	log << "[Recording]Insert a timewaitPkt(framecnt=" << dec << (int)FrameCnt << ")" << endl;
	log << " waitTime = 0x" << hex << (int)WaitTime << endl;
	pthread_mutex_unlock(&mutex);

	RecordFile.write(TIMEWAIT_MSG_HEAD, 1);
	RecordFile.write((const char *)&WaitTime, 4);
	FrameBegin = FrameEnd;
}

/* insert a timesyncPkt */
void InsertSyncPkt()
{
	pthread_mutex_lock(&mutex);
	log << "[Recording]Insert a timesyncPkt(framecnt=" << dec << (int)FrameCnt << ")" << endl;
	pthread_mutex_unlock(&mutex);

}

/* return False if socketset are closed by vnc-client or vnc-server, otherwise return True */
AU_BOOL HandleCTSMsg(SocketSet *c_sockset)
{
    uint32_t c_retnum, encoding_len, text_len;
    uint8_t cts_msg_type;
    uint16_t encoding_num;
    //static uint8_t button_mask = 0;
  
    //read cts_msg_type  
    c_retnum = recv(c_sockset->SocketToClient, cbuf_ptr, 1, 0);
    if(c_retnum == 0){  // socket has been closed by vnc-client
        return False;
    }
    else if(c_retnum != 1){
        error(True, "ERROR: [HandleCTSMsg] recv msg type: ");
    }
    cts_msg_type = *((uint8_t *)cbuf_ptr);

    /*********************************************************
     * AutoGUI will only forward the following message type: *
     *       framebufferupdaterequest,                       *
     *       pointerevent                                    *
     *       keyevent                                        *
     *       clientcuttext                                   *
     *********************************************************/
    switch(cts_msg_type){
        case rfbSetPixelFormat: 
            //read setpixelformat msg
            ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1, HSZ_SET_PIXEL_FORMAT - 1);
            

            /***********************************************************************
             *    This msg must sent to vnc-server,                                *
             *    because the vnc-client will decode the pixel data by this format  *
             ***********************************************************************/
            si.format.bitsPerPixel = (*(uint8_t *)(cbuf_ptr + 4));

            pthread_mutex_lock(&mutex);
            log << "#analyze packages# C-->S:rfbSetPixelFormat              [Forward]" << endl;
			log << "  pixel.format.bitsPerPixel: " << dec << (int)si.format.bitsPerPixel << endl;
            pthread_mutex_unlock(&mutex);
			if(Recording){
				RecordFile.write(RFB_MSG_HEAD, 1);
				RecordFile.write((const char *)cbuf_ptr, HSZ_SET_PIXEL_FORMAT);
				EventCnt++;
			}

            if( !WriteSocket(c_sockset->SocketToServer, cbuf_ptr, HSZ_SET_PIXEL_FORMAT)){
                return False;
            }

            cbuf_ptr += HSZ_SET_PIXEL_FORMAT;

            CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
            break;
        case rfbSetEncodings:
            #ifdef ANALYZE_PKTS
            pthread_mutex_lock(&mutex);
            log << "#analyze packages# C-->S:rfbSetEncodings" << endl;
            pthread_mutex_unlock(&mutex);
            #endif
            //read setencodings msg head
            ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1, HSZ_SET_ENCODING - 1);
            encoding_num = Swap16(*((uint16_t *)(cbuf_ptr + 2)));
            encoding_len = 4*encoding_num;
            cbuf_ptr += HSZ_SET_ENCODING;

            CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) + encoding_len );
            //read setencodings msg data
            ReadSocket(c_sockset->SocketToClient, cbuf_ptr, encoding_len);
            cbuf_ptr += encoding_len;
            break;
        case rfbFramebufferUpdateRequest:
            //read framebufferupdaterequest msg and forward it with msg_type
            ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1, HSZ_FRAME_BUFFER_UPDATE_REQUEST - 1);
             
            #ifdef FORCE_UPDATE
            //force the vnc-server update the framebuffer non-incremental
            //*((uint8_t *)(cbuf_ptr + 1)) = 0;
            #endif

            uint16_t x_pos, y_pos, width, height;
            uint8_t incremental;
            incremental =  *((uint8_t *)(cbuf_ptr + 1));
            x_pos = Swap16( *((uint16_t *)(cbuf_ptr + 2)) );
            y_pos = Swap16( *((uint16_t *)(cbuf_ptr + 4)) );
            width = Swap16( *((uint16_t *)(cbuf_ptr + 6)) );
            height = Swap16( *((uint16_t *)(cbuf_ptr + 8)) );
			
			if(incremental == 0){	//only forward this message when request non-incremental update
            	pthread_mutex_lock(&mutex);
            	log << "#analyze packages# C-->S:rfbFramebufferUpdateRequest    [forward]" << endl;
            	log << "[CTSMsg]        rect: " << dec << (int)x_pos << "," << (int)y_pos;
            	log << dec << "," << (int)width << "," << (int)height;
            	log << "  incremental:" << dec << (int)incremental << endl;
            	pthread_mutex_unlock(&mutex);
			
				if(Recording){
					RecordFile.write(RFB_MSG_HEAD, 1);
					RecordFile.write((const char *)cbuf_ptr, HSZ_FRAME_BUFFER_UPDATE_REQUEST);
					EventCnt++;
				}
				if( !WriteSocket(c_sockset->SocketToServer, cbuf_ptr, HSZ_FRAME_BUFFER_UPDATE_REQUEST) ){
            	    return False;
            	}
			}

            cbuf_ptr += HSZ_FRAME_BUFFER_UPDATE_REQUEST;
            CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
            break;
        case rfbKeyEvent:
            uint32_t cur_key;
            AU_BOOL cur_key_state;

            //read keyevent msg and forward it with msg_type
            ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1, HSZ_KEY_EVENT - 1);
            cur_key = Swap32( *((uint32_t *)(cbuf_ptr+4)) );
			cur_key_state = *((uint8_t *)(cbuf_ptr+1));
            if( cur_key == XK_Control_L || cur_key == XK_Control_R ){
               	CtrlPressed = cur_key_state; 
            	cbuf_ptr += HSZ_KEY_EVENT;
            	CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
				break;
            }
            else if( HandleSpecialKeys(cur_key, CtrlPressed, cur_key_state) ){
            	cbuf_ptr += HSZ_KEY_EVENT;
            	CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
				break;
            }
            else if(Recording){
				if(cur_key_state == True){	//when a key is pressed, capture a frame before forward it.
					CaptureFrame();
					usleep(500*1000);	//sleep 500ms before compute threshold
					WriteThreshold();
					InsertFrameWaitPkt();
					if(!FirstInput){
						InsertTimeWaitPkt();
						InsertSyncPkt();
					}
					else{
						FirstInput = False;
						gettimeofday(&FrameBegin, NULL);
					}
					FrameCnt++;
				}
				RecordFile.write(RFB_MSG_HEAD, 1);
				RecordFile.write((const char *)cbuf_ptr, HSZ_KEY_EVENT);
				EventCnt++;
			}	
			if( !WriteSocket(c_sockset->SocketToServer, cbuf_ptr, HSZ_KEY_EVENT) ){
                return False;
            }

            pthread_mutex_lock(&mutex);
            log << "#analyze packages# C-->S:rfbKeyEvent                    [Forward]" << endl;
			log << "   [current mode] " << "Recording:" << dec << (int)Recording << "Replaying:" << (int)Replaying << endl;
            pthread_mutex_unlock(&mutex);

            cbuf_ptr += HSZ_KEY_EVENT;
            CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
            break;
        case rfbPointerEvent:
            //read pointerevent msg and forward it with msg_type
            ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1, HSZ_POINTER_EVENT - 1);

            pthread_mutex_lock(&mutex);
            log << "#analyze packages# C-->S:rfbPointerEvent                [Forward]" << endl;
            pthread_mutex_unlock(&mutex);

			if(Recording){
            	//only capture the image before events when the button-mask changed.
            	if( button_mask != *((uint8_t *)(cbuf_ptr+1)) ){
                	button_mask = *((uint8_t *)(cbuf_ptr+1));
					CaptureFrame();
					usleep(500*1000);	//sleep 500ms before compute threshold
					WriteThreshold();
					InsertFrameWaitPkt();
					if(!FirstInput){
						InsertTimeWaitPkt();
						InsertSyncPkt();
					}
					else{
						FirstInput = False;
						gettimeofday(&FrameBegin, NULL);
					}
					FrameCnt++;
				}	
				RecordFile.write(RFB_MSG_HEAD, 1);
				RecordFile.write((const char *)cbuf_ptr, HSZ_POINTER_EVENT);
				EventCnt++;
			}
            if( !WriteSocket(c_sockset->SocketToServer, cbuf_ptr, HSZ_POINTER_EVENT) ){
                return False;
            }

            cbuf_ptr += HSZ_POINTER_EVENT;
            CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
            break;
        case rfbClientCutText:
            #ifdef ANALYZE_PKTS
            pthread_mutex_lock(&mutex);
            log << "#analyze packages# C-->S:rfbClientCutText               " << endl;
            pthread_mutex_unlock(&mutex);
            #endif
            //read cuttext msg head 
            ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1, HSZ_CLIENT_CUT_TEXT - 1);
            text_len = Swap32(*((uint32_t *)(cbuf_ptr + 4)));
            cbuf_ptr += HSZ_CLIENT_CUT_TEXT;
            CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) + text_len );
            //read cuttext msg data
            ReadSocket(c_sockset->SocketToClient, cbuf_ptr, text_len);
            cbuf_ptr += text_len;
            break;
        default:     //as the AutoGUI won't forward setencoding message
            error(False, "ERROR: [HandleCTSMsg] This msg-type(%d) should not appear.", cts_msg_type);
    }
    return True;
}

/* The loop which forward data from Client To Server 
 * return True while user quit, otherwise keep going
 */
void *CTSMainLoop(void *sockset)
{
    uint32_t n, i;
    fd_set c_rfds;
    struct timeval tv;
	AU_BOOL timeout = False;
    SocketSet *c_sockset = (SocketSet *)sockset;

    //write back buffer before Main Loop
    WriteClientBuf();

    // Enter Client-To-Server Main Loop
    #ifdef DEBUG
    cout << "Enter Client-To-Server Main Loop" << endl;
    #endif
    while(true){
        FD_ZERO(&c_rfds);
        FD_SET(c_sockset->SocketToClient, &c_rfds);
        tv.tv_sec = 10; //default timeout 10s
        tv.tv_usec = 0;
        n = c_sockset->SocketToClient + 1;
        i = select(n, &c_rfds, NULL, NULL, &tv);
        switch(i){
            case -1:
                perror("ERROR: [CTSMainLoop] select ");
            case 0:
				timeout = True;
                //timeout
                continue;
            case 1:
				timeout = False;
                //socket can be read now
                break;
            default:
                error(False, "ERROR: [CTSMainLoop] select: should not be here.");
        }
        if( !FD_ISSET(c_sockset->SocketToClient, &c_rfds) ){  //check again
            continue;
        }

        // HandleCTSMsg will return False if socketset are closed by vnc-client or vnc-server, otherwise return True
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
    if(server_minor == 8){
        if( send(sockfd, AU_USED_VER_MSG_38, RFBPROTOVER_SZ, 0) != RFBPROTOVER_SZ ){
            error(True, "ERROR: cannot send version massage");
        }
    }
    else if(server_minor == 7){
        if( send(sockfd, AU_USED_VER_MSG_37, RFBPROTOVER_SZ, 0) != RFBPROTOVER_SZ ){
            error(True, "ERROR: cannot send version massage");
        }
    }
    else{
        error(False, "ERROR: server_minor should be 7 or 8");
    }
   
    /* parse the version msg received from client */
    retnum = recv(sockfd, cbuf_ptr, RFBPROTOVER_SZ, 0);
    memcpy(RfbProtoVersion, cbuf_ptr, RFBPROTOVER_SZ);
    cbuf_ptr += retnum;
    if( sscanf(RfbProtoVersion, "RFB %03d.%03d\n", &client_major, &client_minor) != 2 ){
        error(False, "ERROR: not a valid vnc server");
    }
    if(client_major != MajorVersion || (client_minor != MinorVersion && client_minor != MinorVersion + 1) ){
        error(False, "ERROR: sorry, AutoGUI only support RFB v3.7 and v3.8 right now");
    }

    /* send security type message */
    if( send(sockfd, AU_SEC_TYPE_MSG, 2, 0) != 2 ){
        error(True, "ERROR: cannot send security type massage");
    }
    
    /* read security type */
    retnum = recv(sockfd, cbuf_ptr, 1, 0);
    cbuf_ptr += retnum;

    /* send the security result */
    if(server_minor == 8){
        if( send(sockfd, AU_SEC_RESULT, 4, 0) != 4 ){
            error(True, "ERROR: cannot send security result massage");
        }
    }
    

    /* read clientinit message */
    retnum =recv(sockfd, cbuf_ptr, 1, 0);
    cbuf_ptr += retnum;
 
    /* send serverinit message */
    p_si[SZ_SERVER_INIT_MSG - 1] = 7;
    desktopName = ConstName;
    if( send(sockfd, p_si, SZ_SERVER_INIT_MSG, 0) != SZ_SERVER_INIT_MSG ){
        error(True, "ERROR: cannot send serverinit massage");
    }
    if( send(sockfd, desktopName, 7, 0) != 7 ){
        error(True, "ERROR: cannot send desktopName massage");
    }

    cout << "[Step two] Successfully connect to vnc client" << endl << endl;
    return True;
}
