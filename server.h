#ifndef SERVER_H
#define SERVER_H

//extern fd_set svfdset;

/* =========================================================================
* for server to maintain the current connected client
* ========================================================================*/
struct clientList{
        //int id;
        struct sockaddr_in client;
        struct clientList *next;
};

char * currentClientInfo();
int initialServer(int portNum);
void removeFromClientList(char * ip);
void anounceUpdtCL();
void serverCheck(char *info,int srcfd);
#endif
