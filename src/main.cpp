#include "main.h"

ofstream ClientToAU;	//file to write client input date
ofstream ServerToAU;	//file to write server output data
unsigned char server_buf[SERVER_BUF_SZ];	//buffer to hold server output data
unsigned char client_buf[CLIENT_BUF_SZ];	//buffer to hold client input data
unsigned char *sbuf_ptr = server_buf;
unsigned char *cbuf_ptr = client_buf;
uint32_t retnum = 0;	//bytes read from socket
uint32_t next_len = 0;	//the length will be received next, usually specified by the head-package

ServerInitMsg si;
unsigned char ConstName[] = "AutoGUI";
unsigned char *desktopName = ConstName;
unsigned char *p_si = NULL;

uint8_t AU_SEC_TYPE = 1;	//default secure type(no authentication) for AutoGUI
uint8_t AU_SHARED_FLAG = 0; 	//default shared-flag(exclusive) for AutoGUI

/* handle the errors */
void error(AU_BOOL perror_en, const char* format, ...) {
    va_list args;
    char error_msg[1024];
    va_start(args, format);
    vsprintf(error_msg, format, args); 
    va_end(args);

    if (perror_en)
        perror(error_msg);
    else
        cerr << error_msg << endl;
    exit(0);
}

/* produce a hex dump */
void hexdump(unsigned char *p, uint32_t len)
{
    unsigned char *line = p;
    uint32_t i, linelen, offset = 0;

    while(offset < len)
    {
        cout << hex << setfill('0') << setw(4) << offset << ' ';
		
        linelen = len - offset;
	linelen = linelen > 16 ? 16:linelen;

	for(i = 0; i < linelen; i++){
	    cout << hex << setfill('0') << setw(2) << (int)line[i] << ' ';
	}
	for(; i < 16; i++){
	    cout << "   ";
	}

	for(i = 0; i < linelen; i++){
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

/* set a given socket into non-blocking mode */
//not used yet --zhongbin
void SetNonBlocking(int sock)
{
    int flags;
    if( (flags = fcntl(sock, F_GETFL, 0)) < 0){
	flags = 0;
    }
    if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0){
	error(True, "ERROR: cannot set socket into non-blocking");
    }
}

int main(int argc, char *argv[]) 
{
    uint32_t SockToServer, SockToClient, SockListen, server_port, au_port;
    struct hostent *server;

    // parse argv, set server-name, server-port and AutoGUI port
    if (argc < 4){
        error(False, "ERROR: invalid arguments.  Usage is '%s <hostname> <server-port> <AutoGUI-port>'", argv[0]);
    }
    server_port = atoi(argv[2]);
    if(server_port < VNC_PORT_BASE){
        error(False, "ERROR: invalid port number.  Port number should not less then 5900");
    }
    server = gethostbyname(argv[1]);
    if (server == NULL){
        error(False, "ERROR: cannot find host");
    }
    au_port = atoi(argv[3]);
    if(au_port < VNC_PORT_BASE){
        error(False, "ERROR: invalid listenning port number.  Port number should not less then 5900");
    }

    // create sockets
    SockToServer = socket(AF_INET, SOCK_STREAM, 0);
    if (SockToServer < 0){
        error(True, "ERROR: cannot open socket");
    }
    SockListen = socket(AF_INET, SOCK_STREAM, 0);
    if (SockListen < 0){
        error(True, "ERROR: cannot open socket");
    }

    ClientToAU.open("UsrIn.pkts");
    ServerToAU.open("VncOut.pkts");

    // Initialize the connection between AutoGUI and VNC-Server, include handshakes
    if ( !InitToServer(server, server_port, SockToServer) ){
        error(False, "ERROR: cannot complete the init to vnc-server");
    }

    // Listen on the au_port which the vnc-client will connect to
    if( !ListenTcpPort(au_port, SockListen) ){
        error(False, "ERROR: cannot listen on port(%d)", au_port);
    }

    // Wait for vnc-client
    SockToClient = AcceptVncClient(SockListen);
    cout << "A VNC client has been accepted." << endl;
    #ifdef DEBUG
    cout << (int)SockToClient << endl;
    #endif

    // Initialize the connection between AutoGUI and VNC-Client, include handshakes
    if( !InitToClient(SockToClient) ){
        error(False, "ERROR: cannot complete the init to vnc-client");
    }

    // The main loop of AutoGUI
    while(True){
       // if()
    }

    ClientToAU.close();
    ServerToAU.close();

    return 0;
}
