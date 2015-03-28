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

/* write back client buffer */
void WriteClientBuf()
{
    ClientToAU.write((const char*)client_buf, (uint32_t)(cbuf_ptr -
                     client_buf));
    memset(client_buf, 0, CLIENT_BUF_SZ);
    cbuf_ptr = client_buf;
}

/* return True if writeback, otherwise return False */
AU_BOOL CheckClientBuf(uint32_t used)
{
    if (used > CLIENT_BUF_SZ - 100) {
        WriteClientBuf();
        return True;
    }
    return False;
}

/* Check whether this press is special click or not. If it's special click,
 * return True.
 *     peek_result = 1, single-click
 *     peek_result = 2, double-click
 */
AU_BOOL PeekSpecialClick(SocketSet *sockset, uint32_t *peek_result, uint16_t
                         pointer_x, uint16_t pointer_y)
{
    uint32_t len, ret_len;
    AU_BOOL Flag, Peeking;
    unsigned char *peek_buf;
    uint8_t msg_type, peek_state, button_mask;
    uint16_t x_pos, y_pos;
    uint16_t encoding_num = 0;

    len = 0;
    Peeking = Flag = True;
    peek_state = 0;
    peek_buf = (unsigned char *)malloc(1024);
    memset(peek_buf, 0, 1024);

    cout << "start peeking" << endl;
    // sleep 500ms to make sure that if this is the first click of double-click,
    // the press of second click will be peeked usleep(500*1000);
    while (Peeking) {
        if (len > 512) {    // peek too much
            Flag = False;
            len = 0;
            break;
        }
        ret_len = recv(sockset->SocketToClient, peek_buf, len + 1, MSG_PEEK);
        if (ret_len == len) {
            if (peek_state == 0) {
                Flag = False;
            } else {
                Flag = True;
            }
            break;
        }
        if ( ret_len != len + 1 ) {
            error(True, "ERROR: [PeekSpecialClick] recv msg type");
        }
        msg_type = *((uint8_t *)(peek_buf + len));
        switch (msg_type) {
          case rfbSetPixelFormat:
          case rfbClientCutText:
            error(False, "ERROR: [PeekSpecialClick] These message(%d)"
                  "should not appear between pointer press and release.",
                  msg_type);
            break;
          case rfbKeyEvent:
            Flag = False;
            len = 0;
            Peeking = False;
            break;
          case rfbSetEncodings:
            if ( recv(sockset->SocketToClient, peek_buf, len +
                HSZ_SET_ENCODING, MSG_PEEK) != (len + HSZ_SET_ENCODING)) {
                error(True,
                    "ERROR: [PeekSpecialClick] rfb set encoding message");
            }
            encoding_num = Swap16( *((uint16_t *)(peek_buf + len + 2)));
            len += HSZ_SET_ENCODING + encoding_num*4;
            break;
          case rfbFramebufferUpdateRequest:
            if ( recv(sockset->SocketToClient, peek_buf, len +
                HSZ_FRAME_BUFFER_UPDATE_REQUEST, MSG_PEEK) != (len +
                HSZ_FRAME_BUFFER_UPDATE_REQUEST) ) {
                error(True, "ERROR: [PeekSpecialClick] frame buffer update"
                            "request"); }
            len += HSZ_FRAME_BUFFER_UPDATE_REQUEST;
            break;
          case rfbPointerEvent:
            if ( recv(sockset->SocketToClient, peek_buf, len +
                HSZ_POINTER_EVENT, MSG_PEEK) != (len + HSZ_POINTER_EVENT) ) {
                error(True, "ERROR: [PeekSpecialClick] pointerEvent");
            }
            button_mask = *(uint8_t *)(peek_buf + len + 1);
            x_pos = *(uint16_t *)(peek_buf + len + 1 + 1);
            y_pos = *(uint16_t *)(peek_buf + len + 1 + 3);
            pthread_mutex_lock(&mutex);
            log << "[special click]button_mask: " << hex << (int)button_mask
                << " x,y " << (int)x_pos << " " << (int)y_pos
                << " should be: " << (int)pointer_x << " "
                << (int)pointer_y << endl;
            log << "[specialclick]peek_state: " << peek_state << endl;
            pthread_mutex_unlock(&mutex);
            if ( button_mask == 0 && peek_state == 0 && x_pos == pointer_x
                 && y_pos == pointer_y ) {
                peek_state++;
                len += HSZ_POINTER_EVENT;
            } else if ( button_mask == 1 && peek_state == 1
                        && x_pos == pointer_x && y_pos == pointer_y ) {
                peek_state++;
                len += HSZ_POINTER_EVENT;
                Peeking = False;
            } else {
                Flag = False;
                len = 0;
                Peeking = False;
            }
            break;
          default:
            error(False, "ERROR: [PeekSpecialClick] this message(%d) is not"
                         " supported", msg_type);
        }
    }
    cout << "end peeking" << endl;
    cout << "peek_state " << (int)peek_state << endl;
    *peek_result = peek_state;
    free(peek_buf);
    return Flag;
}

/* Initialize threshold list */
void InitThreshold()
{
    uint16_t i;
    string line;
    thres_list_len = 0;

    PixelThresR.open("./framein/fb.thres");
    if (!PixelThresR.is_open()) {
        error(True, "Cannot open PixelThresR to initialize thres_list");
    }
    // compute thres_list_len
    while (getline(PixelThresR, line)) {
        thres_list_len++;
    }
    PixelThresR.close();
    PixelThresR.open("./framein/fb.thres");
    if (!PixelThresR.is_open()) {
        error(True, "Cannot open PixelThresR to initialize thres_list");
    }
    if (thres_list != NULL) {
        free(thres_list);
        thres_list = NULL;
    }
    thres_list = (float *)malloc(sizeof(float) * thres_list_len);
    if (thres_list == NULL) {
        error(True, "Cannot malloc memory for thres_list");
    }
    memset(thres_list, 0, thres_list_len*sizeof(float));

    for (i = 0; i < thres_list_len; i++) {
        PixelThresR >> thres_list[i];
    }
    PixelThresR.close();
    pthread_mutex_lock(&mutex);
    log << "Initialize thres_list. len = " << dec << (int)thres_list_len
        << endl;
    for (i = 0; i < thres_list_len; i++) {
        log << dec << (float)thres_list[i] << endl;
    }
    pthread_mutex_unlock(&mutex);
}

/* Process a single RFB packet read from ReplayFile
 * There following message type are supported now --zhongbin
 */
void HandleRfbPkt(SocketSet *c_sockset)
{
    uint8_t rfb_msg_type;
    unsigned char tmp_msg_buf[HSZ_RFB_MSG_MAX];
    memset(tmp_msg_buf, 0, HSZ_RFB_MSG_MAX);

    ReplayFile.read((char *)&tmp_msg_buf[0], 1);
    if (!ReplayFile.good()) {
        error(False, "ERROR: [HandleRfbPkt] cannot read rfb_msg_type");
    }
    rfb_msg_type = tmp_msg_buf[0];
    switch (rfb_msg_type) {
      case rfbSetPixelFormat:
        pthread_mutex_lock(&mutex);
        log << "[Replaying -> HandleRfbPkt] rfbSetPixelFormat message"
            << endl;
        pthread_mutex_unlock(&mutex);
        ReplayFile.read((char *)&tmp_msg_buf[1], HSZ_SET_PIXEL_FORMAT - 1);
        WriteSocket(c_sockset->SocketToServer, tmp_msg_buf,
                    HSZ_SET_PIXEL_FORMAT);
        break;
      case rfbFramebufferUpdateRequest:
        pthread_mutex_lock(&mutex);
        log << "[Replaying -> HandleRfbPkt] rfbFramebufferUpdateRequest"
            << " message" << endl;
        pthread_mutex_unlock(&mutex);
        ReplayFile.read((char *)&tmp_msg_buf[1],
                        HSZ_FRAME_BUFFER_UPDATE_REQUEST - 1);
        WriteSocket(c_sockset->SocketToServer, tmp_msg_buf,
                    HSZ_FRAME_BUFFER_UPDATE_REQUEST);
        break;
      case rfbKeyEvent:
        pthread_mutex_lock(&mutex);
        log << "[Replaying -> HandleRfbPkt] rfbKeyEvent message" << endl;
        pthread_mutex_unlock(&mutex);
        ReplayFile.read((char *)&tmp_msg_buf[1], HSZ_KEY_EVENT - 1);
        WriteSocket(c_sockset->SocketToServer, tmp_msg_buf, HSZ_KEY_EVENT);
        break;
      case rfbPointerEvent:
        pthread_mutex_lock(&mutex);
        log << "[Replaying -> HandleRfbPkt] rfbPointerEvent message"
            << endl;
        pthread_mutex_unlock(&mutex);
        ReplayFile.read((char *)&tmp_msg_buf[1], HSZ_POINTER_EVENT - 1);
        WriteSocket(c_sockset->SocketToServer, tmp_msg_buf,
                    HSZ_POINTER_EVENT);
        break;
      default:
        error(False, "ERROR: [HandleRfbPkt] This rfb_msg_type(%d)"
                     " should not appear.", rfb_msg_type);
    }
}

/* assignment RfbFrameThres from thres_list */
void GetThreshold()
{
    if (FrameCnt > thres_list_len ) {
        cout << "FrameCnt: " << dec << (int)FrameCnt << " "
             << sizeof(thres_list)/sizeof(float) << endl;
        error(False, "ERROR: [GetThreshold] FrameCnt should less then length"
                     " of thres_list");
    }
    RfbFrameThres = thres_list[FrameCnt];

    pthread_mutex_lock(&mutex);
    log << "[Replaying -> GetThreshold] RfbFrameThres = " << dec
        << RfbFrameThres << endl;
    pthread_mutex_unlock(&mutex);
}

/* assignment CapPixelData by read data from RectFramePixelR */
void GetTargetFrame()
{
    unsigned char bmp_head[54];

    memset(CapPixelData[0], 0, sizeof(CapPixelData));
    memset(bmp_head, 0, sizeof(bmp_head));

    sprintf(pathRectFramePixel, "framein/fb%06u_raw.bmp", FrameCnt);
    cout << "      pathRectFramePixel: " << pathRectFramePixel << endl;
    RectFramePixelR.open(pathRectFramePixel);
    if (!RectFramePixelR.is_open()) {
        error(False, "ERROR: [Replaying -> GetTargetFrame] can not open"
                     " file:%s", pathRectFramePixel);
    }
    // read .bmp file head
    RectFramePixelR.read((char *)bmp_head, sizeof(bmp_head));
    RectFramePixelR.read((char *)CapPixelData[0], sizeof(CapPixelData));
    if ( (RectFramePixelR.rdstate() && (RectFramePixelR.fail() ||
          RectFramePixelR.bad())) != 0) {
        error(True, "[Replaying -> GetTargetFrame] there is something wrong"
                    " when reading replaying file");
    }
    RectFramePixelR.close();

    pthread_mutex_lock(&mutex);
    log << "[Replaying -> GetTargetFrame] RfbFrameThres = " << dec
        << RfbFrameThres << "  FrameCnt = " << (int)FrameCnt << endl;
    pthread_mutex_unlock(&mutex);
}

/* Compare the diff between CapPixelData and CurPixelData
 * Return True is matched(True), otherwise return un-matched(False) */
AU_BOOL TryMatchFrame()
{
    float threshold;
    uint32_t sum_px, num_px_diff;
    sum_px = (RECT_CX - RECT_X) * (RECT_CY - RECT_Y);

    #ifdef DEBUG
    static uint32_t cnt = 0;
    INIT_BMP(tmp2);
    char tmppath[1024];
    memset(tmppath, 0, sizeof(tmppath));
    ofstream tmp;
    #endif

    pthread_mutex_lock(&mutex);
    log << "[Replaying -> TryMatchFrame] Compare the diff between CapPixelData"
        << " and CurPixelData(framecnt=" << dec << (int)FrameCnt << ")"
        << endl;
    num_px_diff = FrameCompare();
    threshold = 1 - ((num_px_diff*1.0)/sum_px);
    log << "   threshold = " << setprecision(6) << threshold << endl;
    log << "   RfbFrameThres = " << RfbFrameThres << endl;
    log << "   num_px_diff = " << dec << (int)num_px_diff << "  sum_px = "
        << (int)sum_px << endl;
    pthread_mutex_unlock(&mutex);

    // the num_px_diff is bigger then expected
    if (threshold < RfbFrameThres) {
        // write .bmp file to record the matching process
        #ifdef DEBUG
        sprintf(tmppath, "tmp%06u.bmp", cnt*2);
        tmp.open(tmppath);
        pthread_mutex_lock(&mutex);
        tmp.write((char *)&tmp2, sizeof(tmp2));
        tmp.write((char *)CurPixelData[0], sizeof(CurPixelData));
        pthread_mutex_unlock(&mutex);
        tmp.close();
        #endif
        return False;
    } else {
        // write .bmp file to record the matching process
        #ifdef DEBUG
        sprintf(tmppath, "tmp%06u.bmp", cnt*2+1);
        tmp.open(tmppath);
        pthread_mutex_lock(&mutex);
        tmp.write((char *)&tmp2, sizeof(tmp2));
        tmp.write((char *)CurPixelData[0], sizeof(CurPixelData));
        pthread_mutex_unlock(&mutex);
        tmp.close();
        cnt++;
        #endif
        return True;
    }
}

/* read a event from recorded file then send it. return True when need
   waitforframe, else return False */
AU_BOOL SendReplayerInput(SocketSet *c_sockset)
{
    uint8_t replay_type;
    uint64_t delaytime, timestamp;
    AU_BOOL WaitForFrame, match;
    WaitForFrame = False;    // defualt value

    if ((ReplayFile.rdstate() && (ReplayFile.fail() || ReplayFile.bad())) != 0) {
        error(True, "ERROR: [Replaying -> SendReplayerInput] reading replay"
                    " file ");
    }
    ReplayFile.read((char *)&replay_type, 1);
    if (!ReplayFile.eof()) {
        switch (replay_type) {
          case rfbPkt:
            pthread_mutex_lock(&mutex);
            log << "[Replaying] handling a rfbPkt" << endl;
            pthread_mutex_unlock(&mutex);
            if ((ReplayFile.rdstate() && (ReplayFile.fail() ||
                 ReplayFile.bad())) != 0) {
                error(True, "ERROR: [Replaying -> SendReplayerInput]"
                            " reading replay file ");
            }
            HandleRfbPkt(c_sockset);
            break;
          case timewaitPkt:
            pthread_mutex_lock(&mutex);
            log << "[Replaying] handling a timewaitPkt" << endl;
            pthread_mutex_unlock(&mutex);
            if ((ReplayFile.rdstate() && (ReplayFile.fail() ||
                ReplayFile.bad())) != 0) {
                error(True, "ERROR: [Replaying -> SendReplayerInput]"
                            " reading replay file ");
            }
            ReplayFile.read((char *)&delaytime, sizeof(uint64_t));
            //usleep(delaytime);    //us
            break;
          case timesyncPkt:
            pthread_mutex_lock(&mutex);
            log << "[Replaying] handling a timesyncPkt" << endl;
            pthread_mutex_unlock(&mutex);
            if ((ReplayFile.rdstate() && (ReplayFile.fail() ||
                ReplayFile.bad())) != 0) {
                error(True, "ERROR: [Replaying -> SendReplayerInput] "
                            "reading replay file ");
            }
            ReplayFile.read((char *)&timestamp, sizeof(uint64_t));
            break;
          case framewaitPkt:
            //cout << "sleep long" << endl;
            // sleep 200ms between two ReplayerInput framewaitPkt
            //usleep(2*1000*1000);
            pthread_mutex_lock(&mutex);
            log << "[Replaying] handling a framewaitPkt" << endl;
            pthread_mutex_unlock(&mutex);
            GetThreshold();
            GetTargetFrame();
            match = TryMatchFrame();
            if (match) {
                // increase after a frame has been matched
                FrameCnt++;
            } else {
                WaitForFrame = True;
            }
            break;
          case ddelayPkt:
            pthread_mutex_lock(&mutex);
            log << "[Replaying] handling a ddelayPkt" << endl;
            pthread_mutex_unlock(&mutex);
            break;
          case chkpointPkt:
            pthread_mutex_lock(&mutex);
            log << "[Replaying] handling a chkpointPkt" << endl;
            pthread_mutex_unlock(&mutex);
            break;
          case exitPkt:
            pthread_mutex_lock(&mutex);
            log << "[Replaying] handling a exitPkt" << endl;
            log << "            exit Replaying mode " << endl;
            pthread_mutex_unlock(&mutex);

            // exit replay mode after a exitPkt appeared
            // and close the ReplayFile at the same time
            Replaying = False;
            ReplayFile.close();
            cout << "[Replaying] exit Replaying mode " << endl;
            break;
          default:
            error(False, "ERROR: [SendReplayerInput] unrecognised replay"
                         " file message type");
        }
        return WaitForFrame;
    } else {
        error(False, "ERROR: [SendReplayerInput] Should close this file when "
                     " handling a exitPkt");
        return False;
    }
}

/* while replaying need to use ReplayFile input instead */
void HandleReplayerInput(SocketSet *c_sockset)
{
    uint32_t n, i;
    fd_set rfds;
    struct timeval tv;
    static AU_BOOL WaitForFrame = False;
    AU_BOOL match;
    ofstream TmpPixelData;
    char pathFrameCheck[1024];
    memset(pathFrameCheck, 0, sizeof(pathFrameCheck));

    if (WaitForFrame) {
        FD_ZERO(&rfds);
        FD_SET(c_sockset->SocketToServer, &rfds);
        tv.tv_sec = 10;
        tv.tv_usec = 0;
        n = c_sockset->SocketToServer + 1;
        i = select(n, &rfds, NULL, NULL, &tv);
        switch (i) {
          case 0:
            // timeout
            GetThreshold();
            GetTargetFrame();
            match = TryMatchFrame();
            if (match) {
                WaitForFrame = False;
                FrameCnt++;
            } else {
                cout << "[Replaying HandleReplayerInput] select"
                     << " timeout(10s). FrameCnt = " << dec
                     << (int)FrameCnt << endl;

                cout << "Write CurPixelData and CapPixelData into .bmp"
                     << " file for check" << endl;
                INIT_BMP(TmpBMPHead);
                sprintf(pathFrameCheck, "framein/fb%06u_raw_Current.bmp",
                        FrameCnt);
                TmpPixelData.open(pathFrameCheck);
                pthread_mutex_lock(&mutex);
                TmpPixelData.write((char *)&TmpBMPHead, sizeof(TmpBMPHead));
                TmpPixelData.write((char *)&CurPixelData[0],
                    sizeof(CurPixelData));
                pthread_mutex_unlock(&mutex);
                TmpPixelData.close();
                sprintf(pathFrameCheck, "framein/fb%06u_raw_Capture.bmp",
                        FrameCnt);
                TmpPixelData.open(pathFrameCheck);
                TmpPixelData.write((char *)&TmpBMPHead, sizeof(TmpBMPHead));
                TmpPixelData.write((char *)&CapPixelData[0],
                    sizeof(CapPixelData));
                TmpPixelData.close();
            }
            break;
          case 1:
            // socket can be read now
	    GetThreshold();
            GetTargetFrame();
            match = TryMatchFrame();
            if (match) {
                WaitForFrame = False;
                FrameCnt++;
            }
            pthread_mutex_lock(&mutex);
            log << "a new pixel data received, TryMatchFrame again."
                << "  WaitForFrame = " << dec << (int)WaitForFrame << endl;

            pthread_mutex_unlock(&mutex);
            break;
          default:
            error(False, "ERROR: [HandleReplayerInput] select"
                         " c_sockset->SocketToServer");
        }
    } else {
        WaitForFrame = SendReplayerInput(c_sockset);
    }
}

/* return True if special keystrokes appears, otherwise return False */
AU_BOOL HandleSpecialKeys(uint32_t cur_key, AU_BOOL CtrlPressed,
                          AU_BOOL cur_key_state)
{
    static AU_BOOL ignore_next_release = False;
    if (CtrlPressed && cur_key_state != False) {
        switch (cur_key) {
          case XK_s:
            if (Replaying) {
                cout << "It's replay mode now, should not send any user"
                     << " input" << endl;
                return True;
            }
            // begin to capture input stream
            Recording = !Recording;
            if (Recording) {
                EventCnt = 0;
                FrameCnt = 0;
                FirstInput = True;
                cout << "####  Start Record  ####" << endl;

                pthread_mutex_lock(&mutex);
                log << "####  Start Record  ####" << endl;
                pthread_mutex_unlock(&mutex);
                RecordFile.open("./framein/icapture_rfb.ibin");
                PixelThres.open("./framein/fb.thres");
                if (!RecordFile.is_open() || !PixelThres.is_open()) {
                    error(True, "Cannot open files for record");
                }
            } else {
                pthread_mutex_lock(&mutex);
                log << "EventCnt captured: " << dec << (int)EventCnt
                    << endl;
                log << "FrameCnt captured: " << dec << (int)FrameCnt
                    << endl;
                log << "####  End Record    ####" << endl;
                pthread_mutex_unlock(&mutex);
                cout << "   insert a exitPkt by default  " << endl;
                cout << "####  End Record    ####" << endl;
                RecordFile.write(EXIT_MSG, 1);
                RecordFile.close();
                PixelThres.close();
            }
            ignore_next_release = True;
            return True;
            break;
          case XK_i:
            if (Recording) {
                cout << "It's record mode now, can not changed into replay"
                     << " mode" << endl;
                return True;
            }
            // begin to replay captured input stream
            Replaying = !Replaying;
            if (Replaying) {
                EventCnt = 0;
                FrameCnt = 0;
                FirstInput = True;
                cout << "####  Start Replay  ####" << endl;

                pthread_mutex_lock(&mutex);
                log << "####  Start Replay  ####" << endl;
                pthread_mutex_unlock(&mutex);
                ReplayFile.open("./framein/icapture_rfb.ibin");
                if (!ReplayFile.is_open()) {
                    error(True, "Cannot open RecordFile for replay");
                }
                InitThreshold();
            }
/*            else {
                pthread_mutex_lock(&mutex);
                log << "EventCnt captured: " << dec << (int)EventCnt << endl;
                log << "FrameCnt captured: " << dec << (int)FrameCnt << endl;
                log << "####  End Replay    ####" << endl;
                pthread_mutex_unlock(&mutex);
                cout << "####  End Replay    ####" << endl;
                ReplayFile.close();
                PixelThresR.close();
            }
*/
            ignore_next_release = True;
            return True;
            break;
          case XK_c:
            // insert checkpoint packet
            cout << "Capture a chkpointPkt" << endl;
            if (Recording) {
                pthread_mutex_lock(&mutex);
                log << "Capture a chkpointPkt" << endl;
                pthread_mutex_unlock(&mutex);
                RecordFile.write(CHECKPOINT_MSG, 1);
            }
            ignore_next_release = True;
            return True;
            break;
          case XK_q:
            // insert exit packet
            cout << "Capture a exitPkt" << endl;
            if (Recording) {
                pthread_mutex_lock(&mutex);
                log << "Capture a exitPkt" << endl;
                pthread_mutex_unlock(&mutex);
                RecordFile.write(EXIT_MSG, 1);
            }
            ignore_next_release = True;
            return True;
            break;
          case XK_d:
            // insert directed delay packet
            cout << "Capture a directed delay packet" << endl;
            if (Recording) {
                pthread_mutex_lock(&mutex);
                log << "Capture a directed delay packet" << endl;
                pthread_mutex_unlock(&mutex);
                RecordFile.write(DDELAY_MSG, 1);
            }
            ignore_next_release = True;
            return True;
            break;
          default:
            return False;
        }
    }
    // ignore the special key release
    if (ignore_next_release == True) {
        ignore_next_release = False;
        return True;
    }
    return False;
}

/* compare CurPixelData and CapPixelData, return the different pixel number
 * this function should been called after pthread_mutex_lock */
uint32_t FrameCompare()
{
    uint32_t num_px_diff;
    uint16_t i, j;
    num_px_diff = 0;
    for (i = RECT_Y; i < RECT_CY; i++) {
        for (j = RECT_X; j < RECT_CX; j++) {
            if (CurPixelData[i][j] != CapPixelData[i][j]) {
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
    memset(CapPixelData[0], 0, sizeof(CurPixelData));
    memcpy(CapPixelData[0], CurPixelData[0], sizeof(CurPixelData));
    log << "[Recording]Capture a frame pixel data(framecnt=" << dec
        << (int)FrameCnt << ")" << endl;
    pthread_mutex_unlock(&mutex);

    sprintf(pathRectFramePixel, "framein/fb%06u_raw.bmp", FrameCnt);
    RectFramePixel.open(pathRectFramePixel);
    if (!RectFramePixel.is_open()) {
        error(True, "Can not open a file for frame pixel data");
    }
    INIT_BMP(bmphead);
    // write .bmp file head
    RectFramePixel.write((char *)&bmphead, sizeof(bmphead));
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
    log << "[Recording]Compute threshold for a captured frame(framecnt=" << dec
        << (int)FrameCnt << ")"  << endl; num_px_diff = FrameCompare();
    /* in our tests, we found out the default(0.95) is too small,
     * use 99% instead.
     * --Perth Charles */
    //threshold = 1 - ((num_px_diff + (sum_px - num_px_diff)*0.05)*1.0)/sum_px;
    threshold = 1 - ((num_px_diff + (sum_px - num_px_diff)*0.03)*1.0)/sum_px;
    //threshold = 1 - (num_px_diff*1.0)/sum_px;
    log << "   threshold = " << setprecision(3) << threshold << endl;
    pthread_mutex_unlock(&mutex);

    PixelThres << setprecision(3) << threshold << endl;
}

/* insert a framewaitPkt */
void InsertFrameWaitPkt()
{
    pthread_mutex_lock(&mutex);
    log << "[Recording]Insert a framewaitPkt(framecnt=" << dec << (int)FrameCnt
        << ")" << endl; pthread_mutex_unlock(&mutex);

    RecordFile.write(FRAMEWAIT_MSG, 1);
}

/* insert a timewaitPkt */
void InsertTimeWaitPkt()
{
    uint64_t WaitTime;    // us
    gettimeofday(&FrameEnd, NULL);
    WaitTime = (FrameEnd.tv_sec - FrameBegin.tv_sec)*1000*1000 +
               (FrameEnd.tv_usec - FrameBegin.tv_usec);

    pthread_mutex_lock(&mutex);
    log << "[Recording]Insert a timewaitPkt(framecnt=" << dec << (int)FrameCnt
        << ")" << endl; log << " waitTime = 0x" << hex << (int)WaitTime << endl;
    pthread_mutex_unlock(&mutex);

    RecordFile.write(TIMEWAIT_MSG_HEAD, 1);
    RecordFile.write((const char *)&WaitTime, sizeof(uint64_t));
    FrameBegin = FrameEnd;
}

/* insert a timesyncPkt */
void InsertSyncPkt()
{
    uint64_t SyncTime = 0;    // insert zero by now --zhongbin
    pthread_mutex_lock(&mutex);
    log << "[Recording]Insert a timesyncPkt(framecnt=" << dec
        << (int)FrameCnt << ")" << endl;
    pthread_mutex_unlock(&mutex);

    RecordFile.write(TIMESYNC_MSG_HEAD, 1);
    RecordFile.write((const char *)&SyncTime, sizeof(uint64_t));
}

/* return False if socketset are closed by vnc-client or vnc-server,
 * otherwise return True */
AU_BOOL HandleCTSMsg(SocketSet *c_sockset)
{
    uint32_t c_retnum, encoding_len, text_len;
    uint8_t  cts_msg_type;
    uint16_t encoding_num;
    // AU_BOOL SpecialClick;
    static struct timeval LastPointerClick = {0, 0}, CurPointerClick;
    uint64_t PointerVal;    // us

    // a default setencoding packet
    unsigned char SetEncode_buf[4 + 4*1] = { \
        '\x02', '\x00', '\x00', '\x01', \
        '\x00', '\x00', '\x00', '\x00', \
    };
    // read cts_msg_type
    c_retnum = recv(c_sockset->SocketToClient, cbuf_ptr, 1, 0);
    if (c_retnum == 0) {  // socket has been closed by vnc-client
        return False;
    } else if (c_retnum != 1) {
        error(True, "ERROR: [HandleCTSMsg] recv msg type: ");
    }
    cts_msg_type = *((uint8_t *)cbuf_ptr);

    /*********************************************************
     * AutoGUI will only forward the following message type: *
     *       rfbSetPixelFormat,                              *
     *       framebufferupdaterequest,                       *
     *       pointerevent                                    *
     *       keyevent                                        *
     *********************************************************/
     switch (cts_msg_type) {
       case rfbSetPixelFormat:
         // read setpixelformat msg
         ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1,
                    HSZ_SET_PIXEL_FORMAT - 1);

         /**********************************************************************
          *    This msg must sent to vnc-server,                               *
          *    because the vnc-client will decode the pixel data by this format*
          *********************************************************************/
         si.format.bitsPerPixel = (*(uint8_t *)(cbuf_ptr + 4));

         pthread_mutex_lock(&mutex);
         log << "#analyze packages# C-->S:rfbSetPixelFormat"
             << "              [Forward]" << endl;
         log << "  pixel.format.bitsPerPixel: " << dec
             << (int)si.format.bitsPerPixel << endl;
         pthread_mutex_unlock(&mutex);
         if (Recording) {
             RecordFile.write(RFB_MSG_HEAD, 1);
             RecordFile.write((const char *)cbuf_ptr, HSZ_SET_PIXEL_FORMAT);
             EventCnt++;
         }
         // ignore the vnc-client usr input when replaying
         if (!Replaying) {
             if ( !WriteSocket(c_sockset->SocketToServer, cbuf_ptr,
                               HSZ_SET_PIXEL_FORMAT)) {
                 return False;
             }
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
         // read setencodings msg head
         ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1,
                    HSZ_SET_ENCODING - 1);
         encoding_num = Swap16(*((uint16_t *)(cbuf_ptr + 2)));
         encoding_len = 4*encoding_num;
         cbuf_ptr += HSZ_SET_ENCODING;

         CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) + encoding_len );
         // read setencodings msg data
         ReadSocket(c_sockset->SocketToClient, cbuf_ptr, encoding_len);

         // send a default message instead, to avoid the tounchkit bug
         // --Perth Charles
         if ( !WriteSocket(c_sockset->SocketToServer, SetEncode_buf,
              sizeof(SetEncode_buf)) ) {
             return False;
         }

         cbuf_ptr += encoding_len;
         break;
       case rfbFramebufferUpdateRequest:
         // read framebufferupdaterequest msg and forward it with msg_type
         ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1,
                    HSZ_FRAME_BUFFER_UPDATE_REQUEST - 1);

         #ifdef FORCE_UPDATE
         // force the vnc-server update the framebuffer non-incremental
         //*((uint8_t *)(cbuf_ptr + 1)) = 0;
         #endif

         uint16_t x_pos, y_pos, width, height;
         uint8_t incremental;
         incremental =  *((uint8_t *)(cbuf_ptr + 1));
         x_pos = Swap16( *((uint16_t *)(cbuf_ptr + 2)) );
         y_pos = Swap16( *((uint16_t *)(cbuf_ptr + 4)) );
         width = Swap16( *((uint16_t *)(cbuf_ptr + 6)) );
         height = Swap16( *((uint16_t *)(cbuf_ptr + 8)) );

         // only forward this message when request non-incremental update
         if (incremental == 0) {
             pthread_mutex_lock(&mutex);
             log << "#analyze packages# C-->S:rfbFramebufferUpdateRequest"
                 << "    [forward]" << endl;
             log << "[CTSMsg]        rect: " << dec << (int)x_pos << ","
                 << (int)y_pos;
             log << dec << "," << (int)width << "," << (int)height;
             log << "  incremental:" << dec << (int)incremental << endl;
             pthread_mutex_unlock(&mutex);

             if (Recording) {
                 RecordFile.write(RFB_MSG_HEAD, 1);
                 RecordFile.write((const char *)cbuf_ptr,
                                  HSZ_FRAME_BUFFER_UPDATE_REQUEST);
                 EventCnt++;
             }
         }
         if ( !WriteSocket(c_sockset->SocketToServer, cbuf_ptr,
                           HSZ_FRAME_BUFFER_UPDATE_REQUEST) ) {
             return False;
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
         if ( cur_key == XK_Control_L || cur_key == XK_Control_R ) {
                CtrlPressed = cur_key_state;
             cbuf_ptr += HSZ_KEY_EVENT;
             CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
             break;
         } else if ( HandleSpecialKeys(cur_key, CtrlPressed, cur_key_state) ) {
             cbuf_ptr += HSZ_KEY_EVENT;
             CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
             break;
         } else if (Recording) {
             // when a key is pressed, capture a frame before forward it.
             if (cur_key_state == True) {
                 CaptureFrame();
                 // sleep 500ms before compute threshold
                 usleep(500*1000);
                 WriteThreshold();
                 InsertFrameWaitPkt();
                 if (!FirstInput) {
                     InsertTimeWaitPkt();
                     InsertSyncPkt();
                 } else {
                     FirstInput = False;
                     gettimeofday(&FrameBegin, NULL);
                 }
                 FrameCnt++;
             }
             RecordFile.write(RFB_MSG_HEAD, 1);
             RecordFile.write((const char *)cbuf_ptr, HSZ_KEY_EVENT);
             EventCnt++;
         }
         // ignore the vnc-client usr input when replaying
         if (!Replaying) {
             if ( !WriteSocket(c_sockset->SocketToServer, cbuf_ptr,
                  HSZ_KEY_EVENT) ) {
                 return False;
             }
         }

         pthread_mutex_lock(&mutex);
         log << "#analyze packages# C-->S:rfbKeyEvent"
             << "                    [Forward]" << endl;
         log << "   [current mode] " << "Recording:" << dec << (int)Recording
             << " Replaying:" << (int)Replaying << endl;
         pthread_mutex_unlock(&mutex);

         cbuf_ptr += HSZ_KEY_EVENT;
         CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
         break;
       case rfbPointerEvent:
         // SpecialClick = False;
         // read pointerevent msg and forward it with msg_type
         ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1,
                    HSZ_POINTER_EVENT - 1);

         pthread_mutex_lock(&mutex);
         log << "#analyze packages# C-->S:rfbPointerEvent"
             << "                [Forward]" << endl;
         pthread_mutex_unlock(&mutex);
         if (Recording) {
             gettimeofday(&CurPointerClick, NULL);
             PointerVal = (CurPointerClick.tv_sec -
                 LastPointerClick.tv_sec)*1000*1000 + (CurPointerClick.tv_usec -
                 LastPointerClick.tv_usec);

             if (PointerVal < POINTERVAL_THRES ) {
                 LastPointerClick = CurPointerClick;
                 button_mask = *((uint8_t *)(cbuf_ptr+1));
             // only capture the image before events when the
             // button-mask changed.
             } else if ( button_mask != *((uint8_t *)(cbuf_ptr+1)) ) {
                 button_mask = *((uint8_t *)(cbuf_ptr+1));
                 // uint16_t pointer_x = *(uint64_t *)(cbuf_ptr + 1 + 1);
                 // uint16_t pointer_y = *(uint64_t *)(cbuf_ptr + 1 + 3);
                 CaptureFrame();
                 // sleep 500ms before compute threshold
                 usleep(500*1000);
                 if (button_mask == 1) {
                     //cout << "get new time" << endl;
                     gettimeofday(&LastPointerClick, NULL);
                 }
                 WriteThreshold();
                 InsertFrameWaitPkt();
                 if (!FirstInput) {
                     InsertTimeWaitPkt();
                     InsertSyncPkt();
                 } else {
                     FirstInput = False;
                     gettimeofday(&FrameBegin, NULL);
                 }
                 FrameCnt++;
             }
             RecordFile.write(RFB_MSG_HEAD, 1);
             RecordFile.write((const char *)cbuf_ptr, HSZ_POINTER_EVENT);
             EventCnt++;
         }
         // ignore the vnc-client usr input when replaying
         if (!Replaying) {
             //cout << "pointer msg:" << endl;
             //hexdump(cbuf_ptr, HSZ_POINTER_EVENT);
             if ( !WriteSocket(c_sockset->SocketToServer, cbuf_ptr,
                  HSZ_POINTER_EVENT) ) {
                 return False;
             }
         }

         cbuf_ptr += HSZ_POINTER_EVENT;
         CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) );
         break;
       case rfbClientCutText:
         #ifdef ANALYZE_PKTS
         pthread_mutex_lock(&mutex);
         log << "#analyze packages# C-->S:rfbClientCutText" << endl;
         pthread_mutex_unlock(&mutex);
         #endif
         // read cuttext msg head
         ReadSocket(c_sockset->SocketToClient, cbuf_ptr + 1,
                    HSZ_CLIENT_CUT_TEXT - 1);
         text_len = Swap32(*((uint32_t *)(cbuf_ptr + 4)));
         cbuf_ptr += HSZ_CLIENT_CUT_TEXT;
         CheckClientBuf( (uint32_t)(cbuf_ptr - client_buf) + text_len );
         // read cuttext msg data
         ReadSocket(c_sockset->SocketToClient, cbuf_ptr, text_len);
         cbuf_ptr += text_len;
         break;
       // as the AutoGUI won't forward setencoding message
       default:
         error(False, "ERROR: [HandleCTSMsg] This msg-type(%d) should not"
                      "appear.", cts_msg_type);
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
    //AU_BOOL timeout = False;
    SocketSet *c_sockset = (SocketSet *)sockset;

    // write back buffer before Main Loop
    WriteClientBuf();

    // Enter Client-To-Server Main Loop
    #ifdef DEBUG
    cout << "Enter Client-To-Server Main Loop" << endl;
    #endif
    while (true) {
        tv.tv_sec = 10; // default timeout 10s
        tv.tv_usec = 0;
        if (Replaying) {
            if (!SocketConnected(c_sockset->SocketToClient)) {
                cout << "user quit" << endl;
                return (void *)True;
            }
            HandleReplayerInput(c_sockset);
            tv.tv_sec = 0;    // don't need to wait for vnc-client input anymore
        }
        FD_ZERO(&c_rfds);
        FD_SET(c_sockset->SocketToClient, &c_rfds);
        n = c_sockset->SocketToClient + 1;
        i = select(n, &c_rfds, NULL, NULL, &tv);
        switch (i) {
          case -1:
            perror("ERROR: [CTSMainLoop] select ");
          case 0:
            //timeout = True;
            // timeout
            continue;
          case 1:
            //timeout = False;
            // socket can be read now
            break;
          default:
            error(False, "ERROR: [CTSMainLoop] select: should not be here.");
        }
        if ( !FD_ISSET(c_sockset->SocketToClient, &c_rfds) ) {  // check again
            continue;
        }

        // HandleCTSMsg will return False if socketset are closed by vnc-client
        // or vnc-server, otherwise return True
        if ( !HandleCTSMsg(c_sockset) ) {
            return (void *)True;
        }
    }
}


/* AutoGUI will listenning on the specified tcp port */
AU_BOOL ListenTcpPort(uint32_t port, uint32_t SockListen)
{
    uint32_t one = 1;
    struct sockaddr_in addr;

    if ( setsockopt(SockListen, SOL_SOCKET, SO_REUSEADDR, &one,
                    sizeof(one)) < 0 ) {
        close(SockListen);
        error(True, "ERROR: ListenTcpPort : setsockopt :");
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if ( bind(SockListen, (struct sockaddr *)&addr, sizeof(addr)) < 0 ) {
        close(SockListen);
        error(True, "ERROR: ListenTcpPort : bind :");
    }

    if ( listen(SockListen, 5) < 0 ) {
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
    if (sock < 0) {
        error(True, "ERROR: AcceptVncClient: accept:");
    }

    return sock;
}

AU_BOOL InitToClient(uint32_t sockfd)
{
    char RfbProtoVersion[RFBPROTOVER_SZ + 1];
    uint32_t client_major, client_minor;

    /* send RFB proto version used by AutoGUI */
    if (server_minor == 8) {
        if ( send(sockfd, AU_USED_VER_MSG_38, RFBPROTOVER_SZ, 0) !=
                  RFBPROTOVER_SZ ) {
            error(True, "ERROR: cannot send version massage");
        }
    } else if (server_minor == 7) {
        if ( send(sockfd, AU_USED_VER_MSG_37, RFBPROTOVER_SZ, 0) !=
                  RFBPROTOVER_SZ ) {
            error(True, "ERROR: cannot send version massage");
        }
    } else {
        error(False, "ERROR: server_minor should be 7 or 8");
    }

    /* parse the version msg received from client */
    retnum = recv(sockfd, cbuf_ptr, RFBPROTOVER_SZ, 0);
    memcpy(RfbProtoVersion, cbuf_ptr, RFBPROTOVER_SZ);
    cbuf_ptr += retnum;
    if ( sscanf(RfbProtoVersion, "RFB %03d.%03d\n", &client_major,
                &client_minor) != 2 ) {
        error(False, "ERROR: not a valid vnc server");
    }
    if (client_major != MajorVersion || (client_minor != MinorVersion &&
        client_minor != MinorVersion + 1) ) {
        error(False, "ERROR: sorry, AutoGUI only supports RFB v3.7 and "
                     "v3.8 right now");
    }

    /* send security type message */
    if ( send(sockfd, AU_SEC_TYPE_MSG, 2, 0) != 2 ) {
        error(True, "ERROR: cannot send security type massage");
    }

    /* read security type */
    retnum = recv(sockfd, cbuf_ptr, 1, 0);
    cbuf_ptr += retnum;

    /* send the security result */
    if (server_minor == 8) {
        if ( send(sockfd, AU_SEC_RESULT, 4, 0) != 4 ) {
            error(True, "ERROR: cannot send security result massage");
        }
    }

    /* read clientinit message */
    retnum =recv(sockfd, cbuf_ptr, 1, 0);
    cbuf_ptr += retnum;

    /* send serverinit message */
    p_si[SZ_SERVER_INIT_MSG - 1] = 7;
    desktopName = ConstName;
    if ( send(sockfd, p_si, SZ_SERVER_INIT_MSG, 0) != SZ_SERVER_INIT_MSG ) {
        error(True, "ERROR: cannot send serverinit massage");
    }
    if ( send(sockfd, desktopName, 7, 0) != 7 ) {
        error(True, "ERROR: cannot send desktopName massage");
    }

    cout << "[Step two] Successfully connect to vnc client" << endl << endl;
    return True;
}
