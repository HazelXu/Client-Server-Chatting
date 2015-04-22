#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>



/* =========================================================================
 * for client to maintain the current connection
 * ========================================================================*/
struct connectionList{
	int id;    
	int fd;                        //file description of the connection
	struct sockaddr_in peer;       //address info of connection
	struct connectionList *next;   //pointer to next
};

char * printConnectionList();
void addToConnectionList(struct sockaddr_in * node, int fd);
void removeFromConnectionList(char * ip);
struct connectionList * getCnnctFromId(int id);
int connected(int cid);

void doSend(int cid, char *msg);
void closeCnnct(int fd);
int connectPeer(char * des,char * port);
int sendCheck(int svfd,char * des, char * port);

void initialClient(int portNum);
#endif