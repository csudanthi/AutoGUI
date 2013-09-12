#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

using namespace std;

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

int main(int argc, char *argv[]) {
    uint32_t sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int32_t retnum;
    // buffer to hold read data
    char read_buffer[256];
    // file to write server output
    ofstream out_file;

    // parse argv, set port and server
    if (argc < 3)
        error(false, "ERROR: invalid arguments.  Usage is '%s <hostname> <port>'", argv[0]);
    portno = atoi(argv[2]);
    server = gethostbyname(argv[1]);
    if (server == NULL)
        error(false, "ERROR: cannot find host");

    // set serv_addr
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    // create socket and connect to server
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error(true, "ERROR: cannot open socket");
    if (connect(sockfd, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error(true, "ERROR: cannot connect to server");

    out_file.open("recv.pkts");
    bool cont = true;
    while (cont) {
        cout << ".";
        cout.flush();
        retnum = read(sockfd, read_buffer, 256);
        if (retnum > 0) {
            out_file.write(read_buffer, retnum);
            out_file.flush();
        } else {
            cont = false;
        }
    }
    out_file.close();
    if (retnum < 0)
        error(true, "ERROR: reading from socket");

  return 0;}
