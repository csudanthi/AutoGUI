#include "main.h"

using namespace std;

/* handle the errors */
void error(bool perror_en, const char* format, ...) {
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
void hexdump(unsigned char *p, unsigned int len)
{
    unsigned char *line = p;
	int i, linelen, offset = 0;

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
void SetNonBlocking(int sock)
{
    int flags;
    if( (flags = fcntl(sock, F_GETFL, 0)) < 0){
	flags = 0;
    }
    if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0){
	error(true, "ERROR: cannot set socket into non-blocking");
    }
}

int main(int argc, char *argv[]) 
{
    uint32_t sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int32_t retnum;
    // buffer to hold read data
    char read_buffer[256];
    // file to write server output
    ofstream out_file;

    // parse argv, set port and server
    if (argc < 3){
        error(false, "ERROR: invalid arguments.  Usage is '%s <hostname> <port>'", argv[0]);
    }
    portno = atoi(argv[2]);
	if(portno < VNC_PORT_BASE){
        error(false, "ERROR: invalid port number.  Port number should not less then 5900");
	}
    server = gethostbyname(argv[1]);
    if (server == NULL){
        error(false, "ERROR: cannot find host");
    }

    // set serv_addr
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
	
    // create socket and connect to server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        error(true, "ERROR: cannot open socket");
    }
    if (connect(sockfd, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        error(true, "ERROR: cannot connect to server");
    }

//    SetNonBlocking(sockfd);

    out_file.open("recv.pkts");
    bool cont = true;
    while (cont) {
        //cout << ".";
        cout.flush();
        retnum = read(sockfd, read_buffer, 256);
	    #ifdef DEBUG
		cout << "retnum:" << retnum << endl;
	    #endif
        if (retnum > 0) {
			#ifdef DEBUG
			cout << "##Received data from server##" << endl;
			hexdump((unsigned char *)read_buffer, retnum);
			#endif

            out_file.write(read_buffer, retnum);
            out_file.flush();
        } else {
            cont = false;
        }
    }
    out_file.close();
    if (retnum < 0){
        error(true, "ERROR: reading from socket");
    }

  	return 0;
  }
