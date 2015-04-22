/* =========================================================================
* server.c :
* -Realize functionality of server:
*	start a server to listen client's request
*	check validation of client connection
*	maintain a client list
* partical codes from Beej's Guide and recitation slides
* http://beej.us/guide/bgnet/output/html/multipage/clientserver.html#simpleserver
* @author Lidan Xu
* @created 9 March 2015
* ========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h> //for get ip
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "REPL.h"
#include "server.h"

fd_set svfdset; //master file discriptor
int fdmax;  //maximum fd number
int serverfd;
static char * PROMPT_SERVER = "server> ";

/* =========================================================================
 * for server checking the validation of connection
 * unable to recognize the second time connection
 * which is after terminate a connection then this pair connect
 * again, it will return a invalid connection
 * ========================================================================*/
struct chattingList{
	char * srcip;  
	char * desip;
	struct chattingList *next;
}*chathead, *curchat;

/*===============================================================================
 *contain the current connected client list
 *================================================================================*/
static struct clientList *clhead;
static struct clientList *clcurrent;

/*===============================================================================
 * get client list info
 * returns a pointer of string
 * ex. "1: timberlake.cse.buffalo.edu 128.205.36.8 4545 2: metallica.timberlake...."
 * will be split by the client itself or LIST command function
 *================================================================================*/
char * currentClientInfo()
{
	struct clientList *p = clhead->next;
	int pn;
	char ip[INET_ADDRSTRLEN],hostname[256],*msg,*port; //*id
	msg = (char *)malloc(sizeof(char)* 300);
	port = (char *)malloc(sizeof(char)* 10);
	//id = (char *)malloc(sizeof(char)* 3);
	memset(msg,0,sizeof(char)* 300);
	memset(port,0,sizeof(char)* 10);
	//memset(id,0,sizeof(char)* 3);
	while (p != NULL){
		inet_ntop(p->client.sin_family,&(p->client.sin_addr),
			ip, INET_ADDRSTRLEN);
		getnameinfo((struct sockaddr *)&(p->client), sizeof(p->client), hostname, sizeof(hostname),
			NULL, 0,0);                      //stored in hostname
		pn = ntohs(p->client.sin_port);
		sprintf(port, "%d", pn);
		//sprintf(id,"%d",p->id);
		//strcat(msg, id);
		//strcat(msg,": ");
		strcat(msg, hostname);
		strcat(msg, " ");
		strcat(msg,ip);
		strcat(msg," ");
		strcat(msg, port);
		strcat(msg," ");
		//printf("msg in function: %s\n",msg);
		p = p->next;
	}
	//free(id);
	free(port);
	return msg;
}//35 18

/*===============================================================================
 * add a new address to client list clhead
 * then set the clcurrent to this node
 *================================================================================*/
void * addToClientList(struct sockaddr_in * node){
	//printf("add CL start.\n");
	struct clientList *newc = (struct clientList *)malloc(sizeof(struct clientList));
	//newc->id = clcurrent->id + 1;
	newc->client = *node;
	newc->next = NULL;
	clcurrent->next = newc;
	clcurrent = newc;
	//printf("add CL end.\n");
}

/*===============================================================================
* remove an address to client list clhead
* keyword is ip address
*================================================================================*/
void removeFromClientList(char * ip){
	char * nodeip[INET_ADDRSTRLEN];
	struct clientList *tmp, *p = clhead;
	int flag = 0;
	while (p->next != NULL){
		inet_ntop(p->next->client.sin_family, &(p->next->client.sin_addr),
			nodeip, INET_ADDRSTRLEN);
		if (strcmp(ip, nodeip) == 0){
			tmp = p->next;
			p->next = tmp->next;
			//printf("port :%d", ntohs(p->next->client.sin_port));
			//printf("id :%d", p->next->id);
			//printf("remove succeed.\n");
			if (clcurrent == tmp){
				clcurrent = p;
			}
			free(tmp);
			flag = 1;
			break;
		}
		else{
			//printf("id in else:%d", p->next->id);
			p = p->next;
		}
	}
	if (flag == 0)
		printf("no such ip.\n");
	/*while (!p->next){
		printf("port :%d", ntohs(p->next->client.sin_port));
		//printf("id :%d", p->next->id);
		//p->next->id = p->next->id - 1;
		p = p->next;
	}*/
	return;
}

/*===============================================================================
* build IPLIST message, send to client
* format: "IPLIST 1: highgate.cse.buffalo.edu ....."
* IPLIST is the key word for client
*================================================================================*/
char * buildIPLISTmsg(){
	//printf("build start.\n");
	char *msg = (char *)malloc(sizeof(char)* 500), *info, tmp[500] = "IPLIST ";
	//printf("msg in function: %s\n", msg);
	info = currentClientInfo(clhead);
	if (strcmp(info, "\0") == 0)
		msg = "IPLIST NONE\0";
	else{
		strcat(tmp, info);
		strcpy(msg, tmp);
		msg[strlen(msg)] = '\0';
	}
	
	//printf("msg: %s\n",msg);
	return msg;
}

/*===============================================================================
 * send to all client the updated client list
 * called when a new client registers or exist client leaves
 *================================================================================*/
void anounceUpdtCL()
{
	char *msg;
	int j,nbytes;
	msg = buildIPLISTmsg();
	//sent new client list info to all
	for (j = 1; j <= fdmax; j++) {
		if (FD_ISSET(j, &svfdset)) {
			//printf("%d sent.\n",j);
			// sent to all except server and new client
			if (j != serverfd)
				if ((nbytes = send(j, msg, strlen(msg), 0)) == -1)
					perror("anounce send");
		}
	}
}

/*===============================================================================
 * initial server
 * reference to the example code Beej's Guide select() function
 *================================================================================*/
int initialServer(int portNum)
{
	fd_set read_fds;
	struct timeval tv;

	int newfd; // socket filedescription, listen，new_file description
	struct sockaddr_in serverAddr;
	struct sockaddr_storage remoteAddr; // client addr info 
	socklen_t addrlen;

	char buff[300];   // client data buffer
	int nbytes;

	char remoteIP[INET_ADDRSTRLEN];

	//--------------------initial client list----------------------
	clhead = (struct clientList *)malloc(sizeof(struct clientList));
	clcurrent = clhead;
	clhead->next = NULL;
	//clhead->id = 0;
	//-------------------------------------------------------------

	//--------------------initial chatting list----------------------
	chathead = (struct chattingList *)malloc(sizeof(struct chattingList));
	curchat = chathead;
	chathead->next = NULL;
	//-------------------------------------------------------------

	const int on = 1;  // for setsockopt() SO_REUSEADDR, below
	int i, j;


	//getIPAddr();
	if ((serverfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket() failed.\n");
	}
	//printf("port: %d\n",portNum);
	memset((void *)&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_UNSPEC;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(portNum);

	if (setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		perror("setsockopt failed\n");
		exit(1);
	}

	if (bind(serverfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
		close(serverfd);
		perror("server: bind failed\n");
	}


	if (listen(serverfd, 10) < 0) {
		perror("listen failed.\n");
		exit(1);
	}
	/*
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
	perror("sigaction");
	exit(1);
	}*/

	printf("server: waiting for connections...\n");
	printf("%s", PROMPT_SERVER);
	fflush(stdout);
	tv.tv_sec = 2;
	tv.tv_usec = 500000;

	FD_ZERO(&svfdset);
	FD_ZERO(&read_fds);
	FD_SET(serverfd, &svfdset);
	FD_SET(0, &svfdset);  //add stdin to fd set

	fdmax = serverfd;

	for (;;) {
		memcpy(&read_fds, &svfdset, sizeof(svfdset));
		//printf("loop in selct.\n");
		if (select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1) {
			perror("select failed.\n");
			exit(4);
		}

		//LISTEN FOR STDIN
		if (FD_ISSET(0, &read_fds)){   //listen to stdin input
			char *input;
			input = (char *)malloc(sizeof(char)* 200);
			fgets(input, 200, stdin);
			if (strcmp(input, "\n"))
			{
				stdineval(parse(input), serverfd, &serverAddr, 0);
				free(input);
			}
			printf("%s", PROMPT_SERVER);
			fflush(stdout);
		}
		else{// LISTEN FOR SOCKETS
			for (i = 1; i <= fdmax; i++) {
				//printf("i in loop:%d\n", i);
				//printf("loop in fdmax.\n");
				if (FD_ISSET(i, &read_fds)) {  //listen to other socket
					if (i == serverfd) {   // handle new connections
						addrlen = sizeof(remoteAddr);
						newfd = accept(serverfd, (struct sockaddr *)&remoteAddr, &addrlen);
						if (newfd == -1) {
							perror("accept failed.\n");
						}
						else {
							char *msg;
							FD_SET(newfd, &svfdset); // add to master set
							if (newfd > fdmax) { // keep track of maximum fd
								fdmax = newfd;
							}
							inet_ntop(remoteAddr.ss_family,
								&(((struct sockaddr_in*)&remoteAddr)->sin_addr),
								remoteIP, INET_ADDRSTRLEN);
							printf("\nnew connection from %s on "
								"socket %d\n", remoteIP, newfd);
							//printf("remote port num:%d\n", ntohs(((struct sockaddr_in*)&remoteAddr)->sin_port));
							//send current clients list
							/*msg = buildIPLISTmsg();
							if ((nbytes = send(newfd, msg, strlen(msg), 0)) < 0)
							perror("send newfd");*/

							addToClientList((struct sockaddr_in *)&remoteAddr);
							anounceUpdtCL();
						}
						printf(("%s", PROMPT_SERVER));
						fflush(stdout);
					}
					else {
						//printf("go into here recv.");
						// data from client
						if ((nbytes = recv(i, buff, sizeof buff, 0)) <= 0) {
							// got error or connection closed by client
							if (nbytes == 0) {
								printf("\nserver: socket %d hung up\n", i);
								getpeername(i, (struct sockaddr *)&remoteAddr, &addrlen);
								inet_ntop(remoteAddr.ss_family,
									&(((struct sockaddr_in*)&remoteAddr)->sin_addr),
									remoteIP, INET_ADDRSTRLEN);
								removeFromClientList(remoteIP);
								FD_CLR(i, &svfdset); // remove from master set
								anounceUpdtCL();
								printf(("%s", PROMPT_SERVER));
								fflush(stdout);
							}
							else {
								perror("recv");
								FD_CLR(i, &svfdset);
							}
							close(i); // bye!
							//FD_CLR(i, &svfdset);
						}
						else {
							buff[nbytes] = '\0';
							//printf("%s\n", buff);
							// deal with data
							recveval(recvparse(buff), i, 0);
							//printf("%s", PROMPT_SERVER);
							//fflush(stdout);
							memset(buff, 0, sizeof(buff));  //clean the buffer!!!!!
						}
					} // END handle data from client
				} // END got new incoming connection
			} // END looping through file descriptors
		}//END if stdin or other socket
	} // END for( ; ; )--and you thought it would never end!

	return 0;
}

/*===============================================================================
* check if nodeip1 and nodeip2 is in chatting
* 1 - in chatting, 0 - not in chatting
*================================================================================*/
int inChatting(char * nodeip1, char * nodeip2)
{
	struct chattingList *p = chathead->next;
	while (p){
		if (strcmp(p->srcip, nodeip1) == 0){
			if (strcmp(p->desip, nodeip2) == 0)
				return 1;
		}
		else if (strcmp(p->srcip, nodeip2) == 0){
			if (strcmp(p->desip, nodeip1) == 0)
				return 1;
		}
		p = p->next;
	}
	return 0;
}

/*===============================================================================
* add a pair of ip to chatting list
*================================================================================*/
void addToChattingList(char * srcip, char * desip)
{
	struct chattingList *newchat = (struct chattingList *)malloc(sizeof(struct chattingList));
	newchat->srcip = srcip;
	newchat->desip = desip;
	newchat->next = NULL;
	curchat->next = newchat;
	curchat = newchat;
}

/*===============================================================================
 * check (des,port) is in Client List or not
 * check (ipstr,des) is in chatting list or not
 * 1 - valid,in CL and not chatting
 * 0 - invalid
 * if valid, add this pair to chatting list
 *================================================================================*/
int inCLnotChatting(char *des, char *port, char *srcip)
{
	struct clientList *p = clhead->next;
	int flag = 0,pn;
	char hostname[50], nodeip[INET_ADDRSTRLEN];

	while (p != NULL){
		//nodeip = inet_ntoa(p->client.sin_addr);
		inet_ntop(p->client.sin_family, &(p->client.sin_addr),
			nodeip, INET_ADDRSTRLEN);  //ip
		if (strcmp(srcip, nodeip) == 0)
			return 0;
		getnameinfo((struct sockaddr *)&(p->client), sizeof(p->client),hostname, sizeof(hostname),
			NULL, 0, 0);               //or hostname
		pn = ntohs(p->client.sin_port);//port num
		if ((strcmp(des, nodeip) == 0) || (strcmp(des, hostname) == 0)){
			if (pn == atoi(port)){
				if (!inChatting(srcip, nodeip)){
					addToChattingList(srcip, nodeip);
					return 1;
				}
				return 0;
			}
		}
		p = p->next;
	}
	return 0;
}

/*===============================================================================
 * analyze info check if valid destination for connection
 * send result to srouce fd
 *================================================================================*/
void serverCheck(char *info, int srcfd)
{
	char *des, *port, *msg, flag[100] = "VALID ";
	int n;
	socklen_t len;
	struct sockaddr_storage src;
	struct sockaddr_in *s;
	char ipstr[INET_ADDRSTRLEN];
	//int portn;

	//-------------get src ip address-------------
	len = sizeof src;
	getpeername(srcfd, (struct sockaddr*)&src, &len);

	s = (struct sockaddr_in *)&src;
	inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
	//printf("Peer IP address: %s\n", ipstr);
	//printf("Peer port      : %d\n", portn);
	//--------------------------------------------

	//if (FD_ISSET(srcfd, &svfdset)) {
	des = strtok_r(info, " ", &port);
	if (inCLnotChatting(des, port,ipstr)){
		msg = (char *)malloc(sizeof(char)* 300);
		strcat(flag, des);
		strcat(flag, " ");
		strcat(flag, port);
		strcpy(msg, flag);
		msg[strlen(msg)] = '\0';
		//printf("msg in check server: %s\n", msg);
		if ((n = send(srcfd, msg, strlen(msg), 0)) < 0)
			perror("send in server check");
		//else
		//	printf("%d bytes sent.\n", n);
	}
	else{
		msg = "INVALID";
		if ((n = send(srcfd, msg, strlen(msg), 0)) < 0)
			perror("send in server check");
		//else
		//	printf("%d bytes sent.\n", n);

	}
	/*}
	else
	printf("srcfd invalid.\n");*/
}

