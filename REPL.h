#ifndef REPL_H
#define REPL_H

#include <sys/types.h>
#include <sys/socket.h>

extern char *para[];

void prepareServer(char * portNum);;
void prepareClient(char * portNum);
void Error(char * ErrInfo);
void stdineval(const char * ins, int svfd, struct sockaddr_in *addrInfo, const int option);
void recveval(const char * ins, int fd, const int option);
char * parse(char * input);
char *recvparse(char * buf);
int getIP(char * des);

#endif
