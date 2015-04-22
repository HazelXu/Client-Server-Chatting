/* =========================================================================
* client.c :
* Realize functionality of client:
* register to server, connect to other pair, send message to other pair
* @author Lidan Xu
* @created 9 March 2015
* ========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "client.h"
#include "REPL.h"

extern char *para[];

#define MAXDATASIZE 501 //1 for last character '\0'

fd_set clfdset;
int fdmax;
static char * PROMPT_CLIENT = "client> ";

static struct connectionList *clhead;
static struct connectionList *clcurrent;

/*===============================================================================
 *print current connection list info
 *================================================================================*/
char * printConnectionList()
{
	char *token, *s;
	struct connectionList *p = clhead->next;
	char ip[INET_ADDRSTRLEN], hostname[256];
	int pn;

	while (p != NULL){
		inet_ntop(p->peer.sin_family, &(p->peer.sin_addr),
			ip, INET_ADDRSTRLEN);
		getnameinfo((struct sockaddr *)&(p->peer), sizeof(p->peer), hostname, sizeof(hostname),
			NULL, 0, 0);                      //stored in hostname
		pn = ntohs(p->peer.sin_port);
		printf("%d:    ",p->id);
		printf("%-33s", hostname);
		printf("%-16s", ip);
		printf("%d\n", pn);
		p = p->next;
	}
	printf("\n");
	fflush(stdout);
	
}

/*===============================================================================
* add a new address to connection list clhead
* then set the clcurrent to this node
*================================================================================*/
void addToConnectionList(struct sockaddr_in * node,int fd){
	//printf("add CL start.\n");
	struct connectionList *newc = (struct connectionList *)malloc(sizeof(struct connectionList));
	newc->id = clcurrent->id + 1;
	newc->peer = *node;
	newc->fd = fd;
	newc->next = NULL;
	clcurrent->next = newc;
	clcurrent = newc;
	//printf("add CL end.\n");
}

/*===============================================================================
* remove an address to connection list clhead
* keyword is ip address
*================================================================================*/
void removeFromConnectionList(char * ip){
	char nodeip[INET_ADDRSTRLEN];
	struct connectionList *tmp, *p = clhead;
	int flag = 0, i = 0;
	while (p->next != NULL){
		inet_ntop(p->next->peer.sin_family, &(p->next->peer.sin_addr),
			nodeip, INET_ADDRSTRLEN);
		if (strcmp(ip, nodeip) == 0){
			tmp = p->next;
			p->next = tmp->next;
			printf("\nterminated %s.\n",nodeip);
			if (clcurrent == tmp){
				clcurrent = p;
			}
			free(tmp);
			flag = 1;
			break;
		}
		else
			p = p->next;
	}
	if (flag == 0)
		printf("no such ip.\n");
	//-------------------reset id-------------------
	p = clhead;
	while (p->next != NULL){
		i++;
		//printf("port :%d", ntohs(p->next->peer.sin_port));
		//printf("id :%d", p->next->id);
		p->next->id = i;
		p = p->next;
	}
	//----------------------------------------------
	return ;
}

/*===============================================================================
 * close file description fd
 * called when reveive a terminate quest
 *================================================================================*/
void closeCnnct(int fd)
{
	close(fd); // bye!
	FD_CLR(fd, &clfdset); // remove from clfdset set
}

/*===============================================================================
 * get a connection list node from connection list
 *================================================================================*/
struct connectionList * getCnnctFromId(int id)
{
	struct connectionList *tmp, *p = clhead->next;
	int flag = 0;
	while (p != NULL){
		if (p->id == id)
			return p;
		else
			p = p->next;
	}
	Error("no such id");
	return NULL;
}

/*===============================================================================
 * send msg to server, check validation of the connection to des
 *================================================================================*/
int sendCheck(int svfd,char * des, char * port)
{
	//send msg ""CHECK des port"
	char * msg = (char *)malloc(sizeof(char)* 300), *tmp=(char *)malloc(sizeof(char)* 300);
	char ins[500] = "CHECK ";
	memset((void *)tmp, 0, sizeof(char)* 300);
	strcat(tmp, des);
	strcat(tmp, " ");
	strcat(tmp, port);
	strcat(ins, tmp);
	strcpy(msg, ins);
	if (send(svfd, msg, strlen(msg), 0) <= 0){
		perror("send in send check");
		free(msg);
		free(tmp);
		return -1;
	}
	free(msg);
	free(tmp);
	return 0;
}

/*===============================================================================
 * called by connectTo() in REPL.c
 * create a new socket to connect the peer
 *================================================================================*/
int connectPeer(char * des, char * port)
{
	//printf("get into connect peer.\n");
	int sockfd, numbytes, i;
	char buf[101], ip[INET_ADDRSTRLEN];
	struct sockaddr_in clientAddr, remoteAddr;
	struct addrinfo hints, *peerinfo;
	char s[INET_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;


	getaddrinfo(des, port, &hints, &peerinfo);

	if ((sockfd = socket(peerinfo->ai_family, peerinfo->ai_socktype, peerinfo->ai_protocol)) < 0){
		perror("socket() failed.\n");
	}

	if (connect(sockfd, peerinfo->ai_addr, peerinfo->ai_addrlen) < 0) {
		close(sockfd);
		perror("client: connect failed");
	}

	freeaddrinfo(peerinfo);

	FD_SET(sockfd, &clfdset); // add to clfdset set
	if (sockfd > fdmax) { // keep track of maximum fd
		fdmax = sockfd;
	}


}

/*===============================================================================
 * register to server and start listen and receive 
 *================================================================================*/
void RegisterServer(char *servipaddr, char * sportNum,int cportNum){
	//------------for fd_set------------
	fd_set read_fds;
	//int fdmax;  //maximum fd number
	struct timeval tv;
	//-----------------------------------
	
	//-----------for socket--------------
	int sockfd, newfd, listenfd, numbytes,i;
	char buf[MAXDATASIZE], ip[INET_ADDRSTRLEN],remoteIP[INET_ADDRSTRLEN];
	struct sockaddr_in clientAddr, lt;
	struct sockaddr_storage remoteAddr;
	struct addrinfo hints, *servinfo;
	char s[INET_ADDRSTRLEN];
	int on = 1;  // for setsockopt
	socklen_t addrlen;
	int status;


	getIP(ip);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	memset(&clientAddr, 0, sizeof(clientAddr));
	clientAddr.sin_family = AF_UNSPEC;
	clientAddr.sin_addr.s_addr = inet_addr(ip);
	clientAddr.sin_port = htons(cportNum);

	
	memset(&lt, 0, sizeof(lt));
	lt.sin_family = AF_UNSPEC;
	lt.sin_addr.s_addr = htonl(INADDR_ANY);
	lt.sin_port = htons(cportNum);
	//printf("port: %s",sportNum);
	//int pn = atoi(sportNum);
	if ((status = getaddrinfo(servipaddr, sportNum, &hints, &servinfo)) != 0)
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));

	if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) < 0){
		perror("socket()1 failed.\n");
	}
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket()2 failed.\n");
	}
	//for "Address already in use"
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		perror("setsockopt1 failed\n");
		exit(1);
	}

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		perror("setsockopt2 failed\n");
		exit(1);
	}

	if (bind(sockfd, (struct sockaddr *) &clientAddr, sizeof(clientAddr)) < 0) {
		close(sockfd);
		perror("server: bind1 failed\n");
	}

	if (bind(listenfd, (struct sockaddr *) &lt, sizeof(lt)) < 0) {
		close(listenfd);
		perror("server: bind2 failed\n");
		}


	if (connect(sockfd, servinfo->ai_addr,servinfo->ai_addrlen) < 0) {
		close(sockfd);
		perror("client: connect failed");
	}

	inet_ntop(servinfo->ai_family, &(((struct sockaddr_in *)servinfo->ai_addr)->sin_addr), s, sizeof s);
	//inet_ntop(p->client.sin_family, &(p->client.sin_addr),
	//	ip, INET_ADDRSTRLEN);
	printf("client: connecting to %s\n", s);
	//freeaddrinfo(servinfo);

	//--------------------initial connection list----------------------
	clhead = (struct connectionList *)malloc(sizeof(struct connectionList));
	clcurrent = clhead;
	clhead->next = NULL;
	clhead->id = 1;    //start from 1 since id:1 is server timberlake
	//-----------------------------------------------------------------

	//-----------------for other peer connection-----------------------
	if (listen(listenfd, 5) < 0) {
		perror("listen failed.\n");
		exit(1);
	}
	//------------------------------------------------------------------

	//---------------fd set for select----------------------------------
	FD_ZERO(&clfdset);
	FD_ZERO(&read_fds);

	printf("%s", PROMPT_CLIENT);
	fflush(stdout);
	tv.tv_sec = 2;
	tv.tv_usec = 500000;

	FD_SET(0, &clfdset);  //add stdin to fd set
	FD_SET(sockfd, &clfdset);
	FD_SET(listenfd, &clfdset);
	fdmax = sockfd > listenfd ? sockfd : listenfd;

	//------------------------------------------------------------------

	//=====================start waiting for I/O========================
	for (;;) {
		memcpy(&read_fds, &clfdset, sizeof(clfdset));

		if (select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1) {
			perror("select failed.\n");
			exit(4);
		}
		//LISTEN FOR STDIN
		if (FD_ISSET(0, &read_fds)){
			char *input;
			input = (char *)malloc(sizeof(char)* 200);
			fgets(input, 200, stdin);
			if (strcmp(input, "\n"))
			{
				stdineval(parse(input), sockfd, &clientAddr, 1);
				free(input);
			}
			printf("%s", PROMPT_CLIENT);
			fflush(stdout);
		}
		else {  //LISTEN FOR OTHER SOCKET
			for (i = 0; i <= fdmax; i++) {
				//CONNECT underground.cse.buffalo.edu 6784
				//REGISTER timberlake.cse.buffalo.edu 11111
				if (FD_ISSET(i, &read_fds)) {
					if (i == listenfd) {
						// handle new connections
						addrlen = sizeof(remoteAddr);
						newfd = accept(listenfd, (struct sockaddr *)&remoteAddr, &addrlen);
						if (newfd == -1) {
							perror("accept failed.\n");
						}
						else {
							char *msg;
							FD_SET(newfd, &clfdset); // add to clfdset set
							if (newfd > fdmax) { // keep track of maximum fd
								fdmax = newfd;
							}
							inet_ntop(remoteAddr.ss_family,
								&(((struct sockaddr_in*)&remoteAddr)->sin_addr),
								remoteIP, INET_ADDRSTRLEN);
							printf("\nnew connection from %s on "
								"socket %d\n", remoteIP, newfd);
							addToConnectionList((struct sockaddr_in*)&remoteAddr,newfd);
							//send current clients list
							msg = "SUCCESS";
							send(newfd, msg, strlen(msg), 0);
						}
						printf(("%s", PROMPT_CLIENT));
						fflush(stdout);
					}
					else {
						// data from server or peers
						if ((numbytes = recv(i, buf, MAXDATASIZE - 1, 0)) <= 0) {
							// got error or connection closed by client
							if (numbytes == 0) {
								printf("selectserver: socket %d hung up\n", i);
							}
							else {
								perror("recv");
							}
							close(i); // bye!
							FD_CLR(i, &clfdset); // remove from clfdset set
						}
						else {
							buf[numbytes] = '\0';
							//printf("\nreceive from server: %s\n", buf);
							// deal with data
							recveval(recvparse(buf), i, 1);
							printf("%s", PROMPT_CLIENT);
							fflush(stdout);
							memset(buf, 0, sizeof(buf));
						}
					} // END handle data from client
				} // END got new incoming connection
			} // END looping through file descriptors
		}//END if stdin or socket
	} // END for( ; ; )--and you thought it would never end!
	printf("out loop now, yyyyy?\n");
	return 0;
}

/*===============================================================================
 * called by prepareClient();
 * Register first otherwise error
 *================================================================================*/
void initialClient(int portNum){

	while (1){
		printf("%s", PROMPT_CLIENT);
		fflush(stdout);
		char *input, *ins;
		input = (char *)malloc(sizeof(char)* 200);
		fgets(input, 200, stdin);
		if (strcmp(input,"\n") != 0){
			ins = parse(input);

			if (strcmp(ins, "REGISTER") != 0){
				Error("Please Register First");
			}
			else{
				RegisterServer(para[0], para[1], portNum);
				free(input);
				break;
			}
		}
		free(input);
	}
	return 0;
}

/*===============================================================================
 * check if the cid is connected
 * 1 - connected, 0 - not connected
 *================================================================================*/
int connected(int cid)
{
	struct connectionList *p = clhead;
	while (p->next){
		if (cid == p->next->id)
			return 1;
		p = p->next;
	}
	return 0;
}

void doSend(int cid, char *msg)
{
	struct connectionList *p = clhead->next;
	//char *tmp = (char *)malloc(sizeof(char)* 300);
	//memset((void *)tmp, 0, sizeof(char)* 300);
	char ins[500] = "MSG ";
	while (p){
		if (cid == p->id){
			//strcat(tmp, msg);
			strcat(ins, msg);
			strcpy(msg, ins);
			//printf("msg : %s\n",msg);
			if (send(p->fd, msg, strlen(msg), 0) <= 0){
				perror("send in send check");
				return ;
			}
			else{
				printf("Message send to %d\n",cid);
			}
		}
		p = p->next;
	}
	//free(tmp);
	return 0;
}