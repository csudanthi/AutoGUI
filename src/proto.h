
/* main.cpp */
void error(AU_BOOL perror_en, const char* format, ...);
void hexdump(unsigned char *p, unsigned int len);
void SetNonBlocking(int sock);

/* server.cpp */
AU_BOOL InitToServer(struct hostent *server, uint32_t portno, uint32_t sockfd);

