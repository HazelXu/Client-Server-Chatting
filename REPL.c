/* =========================================================================
* REPL.c :
* The whole server/client reply system
* Reply command from stdin for both clients and server
* Reply command from other connected socket
* @author Lidan Xu
* @created 9 March 2015
* ========================================================================*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h> 
#include "REPL.h"
#include "server.h"
#include "client.h"

//extern fd_set svfdset;
//extern fd_set clfdset;

char *para[2];  //parameter list of command
static char * message;
const char * split = " \t\n\0";

void Error(char * ErrInfo)
{
	printf("\n>>>>>>>> %s<<<<<<<<\n",ErrInfo);
}



/*===============================================================================
* use structure ifconf and ifreq and i/o management function
*================================================================================*/
int getIP(char * des){
	//first failed method try to use public DNS, however, it needs knowledge of
	//DNS format
	/*struct addrinfo hints, *res;
	int sockfd;
	int numbytes;
	char buf[100];
	// getaddrinfo() -> public DNS "8.8.8.8"

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if (getaddrinfo("8.8.8.8", "53", &hints, &res) < 0)
	perror("get ip getinfo failed");

	if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
	perror("get ip socket failed");
	}

	//free(res);

	if ((sendto(sockfd, "get ip", strlen("get ip"),0, res->ai_addr, res->ai_addrlen)) == -1) {

	perror("get ip send to failed");
	exit(1);
	}
	printf("msg sent.\n");
	fflush(stdout);
	if ((numbytes = recvfrom(sockfd, buf, 99, 0, res->ai_addr, res->ai_addrlen)) < 0){
	perror("get ip recv failed");
	}

	buf[numbytes] = '\0';
	printf("recv msg: %s\n", buf);
	fflush(stdout);
	close(sockfd);
	*/

	//ANOTHER failed method
	/*int inet_sock;
	struct ifreq ifr;

	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);

	//获得接口地址
	bzero(&ifr, sizeof(ifr));
	strcpy(ifr.ifr_name, "eth0");
	if (ioctl(inet_sock, SIOCGIFADDR, &ifr) < 0)
	perror("ioctl");
	printf("host:%s/n", inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr));
	*/
	int i = 0;
	int sockfd;
	struct ifconf ifconf;
	char buf[512];
	struct ifreq *ifreq;
	char* ip;

	//ifconf initial
	ifconf.ifc_len = 512;
	ifconf.ifc_buf = buf;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0))<0)
	{
		return -1;
	}
	ioctl(sockfd, SIOCGIFCONF, &ifconf);    //io management function
	close(sockfd);
	//get IP address
	ifreq = (struct ifreq*)buf;
	for (i = (ifconf.ifc_len / sizeof(struct ifreq)); i>0; i--)
	{
		ip = inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr);
		if (strcmp(ip, "127.0.0.1") == 0)  //exclude 127.0.0.1
		{
			ifreq++;
			continue;
		}
		strcpy(des, ip);
		//printf("IP: %s", ip);
		return 0;
	}
	return -1;

}

/* =========================================================================================
 * print server-IP-list of stdin command for client
 * input format:"timberlake.cse.buffalo.edu 128.205.36.8 4545 metallica.timberlake...."
 * change input format and print it as a table
 ===========================================================================================*/
void printIPlist(char * str){
	char *token, *s;
	int i = 1;
	printf("ID:        Hostname                     IP Address    PortNum\n");
	if (strcmp(str, "NONE") == 0)
		printf("NO CLIENTS CONNECTED.\n\n");
	else {
		token = strtok_r(str, split, &s); //hostname
		//if (token != NULL)
		//	printf("%3s", token);
		while (token != NULL){
			//if (flag){
			printf("%d:    ", i);
			//}
			//token = strtok_r(NULL, " ", &s);  
			printf("%-33s", token);
			token = strtok_r(NULL, " ", &s);  //ip address
			printf("%-16s", token);
			token = strtok_r(NULL, " ", &s);  //port num
			printf("%s\n", token);
			if (!s)
				break;
			token = strtok_r(NULL, " ", &s);
			i++;
		}
	}
	printf("\n");
}

//============================command function====================================
void help(int option)
{
	printf(">>>    ");
	printf( "-MYIP	Obtain the IP address.\n"
	"       -MYPORT  The port number the socket is listening to.\n"
	"       -REGISTER <SERVER_IP> <PORT_NO>  Register the client with server.\n"
	"       -CONNECT <DESTINATION> <PORT_NO>  Establish connection between two clients.\n"
	"       -LIST  Display all connection of this process.\n"
	"       -TERMINATE <CONNECTION_ID>  Terminate the connection under its id.\n"
	"       -EXIT  Close all connections and terminate the process.\n"
	"       -SEND <CONNECTION_ID> <MSG>  Send a message to the host under its id.\n");
	fflush(stdout);
}

void getMyIP(int option)
{
	char ip[INET_ADDRSTRLEN],hostname[128];
	//inet_ntop(AF_INET,&(addrInfo->sin_addr),ip,INET_ADDRSTRLEN);
	//if (option == 0){
	//gethostname(hostname, sizeof hostname);
		if (getIP(ip) < 0)
			printf("get ip failed.\n");
	//}
	printf(">>>My IP Address: %s\n",ip);
	fflush(stdout);
}

void getMyPort(struct sockaddr_in *addrInfo)
{
	//getpeername(fd, (struct sockaddr *)addrInfo, sizeof(struct sockaddr));
	printf(">>> My Port Number: %d\n",ntohs(addrInfo->sin_port));
	fflush(stdout);
}

void quit(int option, int fd)
{
	exit(EXIT_SUCCESS);
}

void listMyConnection(int option, int fd)
{
	char *listInfo, ip[INET_ADDRSTRLEN], *port, *servname;
	port = (char *)malloc(sizeof(char)* 10);
	socklen_t len;
	if (option == 0){
		printf("\nSERVER-IP-LIST:\n");
		listInfo = currentClientInfo();
		if (strcmp(listInfo, "\0") == 0)
			listInfo = "NONE";
		printIPlist(listInfo);
	}
	else{
		//-----print server first--------
		struct sockaddr_in c;
		len = sizeof(struct sockaddr);
		getpeername(fd, (struct sockaddr *)&c, &len);
		inet_ntop(c.sin_family, &(c.sin_addr), ip, INET_ADDRSTRLEN);
		printf("\nCURRENT CONNECTION LIST:\n");
		printf("ID:        Hostname                     IP Address    PortNum\n");
		printf("1:    ");
		servname = "timberlake.cse.buffalo.edu";
		printf("%-33s", servname);
		printf("%-16s", ip);
		printf("%d\n", ntohs(c.sin_port));
		printConnectionList();
	}
	fflush(stdout);
}

//para[0] -- connection id number
/* has problem when terminate a connection which is from 
   other client connect to this client
   in another word, this command just work for the client who
   established a connection, still didnt fix*/
void tmntConnection(int svfd)
{
	int cid = atoi(para[0]);
	struct connectionList *c;
	char ip[INET_ADDRSTRLEN], *msg;

	if (!para[0]){
		Error("NEED MORE PARAMETER.");
		return;
	}
	if (connected(cid))
		c = getCnnctFromId(cid);
	else{
		Error("Invalid connection number");
	}
	inet_ntop(c->peer.sin_family, &(c->peer.sin_addr), ip, INET_ADDRSTRLEN);
	msg = "TERMINATERECV";
	if (send(c->fd, msg, strlen(msg), 0) < 0)
		perror("quit send");
	//closeCnnct(c->fd);
	removeFromConnectionList(ip);
	//printf("help.\n");
	//fflush(stdout);
}


//para[0] -- connection id number
//para[1] -- msg
void sendMsg()
{
	int cid = atoi(para[0]);
	if (!connected(cid)){
		Error("Invalid connection id number");
		return;
	}
	if (strlen(para[1]) > 100){
		Error("Message is too long");
		return;
	}
	doSend(cid,para[1]);
}

//multiple registration handler
void registerError()
{
	Error("Cannot register twice");
	fflush(stdout);
}

//para[0] -- destination
//para[1] -- port num
void connectTo(int svfd)
{
	if ((para[0] == NULL) || (para[1] == NULL)){
		Error("NEED MORE PARAMETER");
	}
	else{
		sendCheck(svfd, para[0], para[1]);
	}
	fflush(stdout);
}

void Valid()
{
	char *des, *port;
	des = strtok_r(message, " ", &port);
	if (connectPeer(des,port)<0)
		printf("WTF.\n");
	fflush(stdout);
}

void Invalid()
{
	Error("Invalid address or port number");
	fflush(stdout);
}

void showSccss(int fd)
{
	socklen_t len;
	struct sockaddr_storage src;
	len = sizeof src;
	getpeername(fd, (struct sockaddr*)&src, &len);
	char hostname[256];
	getnameinfo((struct sockaddr *)&src, sizeof(src), hostname, sizeof(hostname),
		NULL, 0, 0);
	addToConnectionList((struct sockaddr_in *)&src, fd);
	printf("\nSucceed to connect client: %s\n",hostname);
	fflush(stdout);
}

//receive terminate command
void receiveTmnt(int fd){
	char *ip;
	struct sockaddr_in c;
	socklen_t len;
	len = sizeof c;

	getpeername(fd, (struct sockaddr *)&c, &len);
	inet_ntop(c.sin_family, &(c.sin_addr), ip, INET_ADDRSTRLEN);
	removeFromConnectionList(ip);
	//closeCnnct(fd);
	fflush(stdout);
}

void receiveMsg(int fd)
{
	char ip[INET_ADDRSTRLEN], hostname[256];
	socklen_t len;
	int pn;
	struct sockaddr_in c;
	len = sizeof(struct sockaddr);

	getpeername(fd, (struct sockaddr *)&c, &len);
	inet_ntop(c.sin_family, &(c.sin_addr), ip, INET_ADDRSTRLEN);
	getnameinfo((struct sockaddr *)&c, sizeof(c), hostname, sizeof(hostname),
		NULL, 0, 0);
	pn = ntohs(c.sin_port);
	printf("\nMessage received from %s\n",hostname);
	printf("Sender's IP: %s\n", ip);
	printf("Sender's Port: %d\n", pn);
	printf("Message \"%s\"\n", message);
	printf("\n");
	
}

void displayIPList()
{
	printf("\nUPDATED REGISTERED CLIENT LIST:\n");
	printIPlist(message);
	fflush(stdout);
}

void checkValDes(fd)
{
	serverCheck(message,fd);
	fflush(stdout);
}

//======================end of command function===========================


/* =====================================================================
 * ins is the instruction from user input
 * addrInfo cantains host address info
 * svfd is the server file description
 * option: 0 -- run on server
           1 -- run on client
 * ====================================================================*/
void stdineval(const char * ins, int svfd, struct sockaddr_in *addrInfo,const int option){
	if (strcmp(ins, "HELP") == 0)				{ help(option); }
	else if (strcmp(ins, "MYIP") == 0)			{ getMyIP(option); }
	else if (strcmp(ins, "MYPORT") == 0)			{ getMyPort(addrInfo); }
	else if (strcmp(ins, "EXIT") == 0)			{ quit(option,svfd); }
	else if (strcmp(ins, "LIST") == 0)			{ listMyConnection(option,svfd); }
	else if (option){  // instructions only run on client
		if (strcmp(ins, "TERMINATE") == 0)       { tmntConnection(svfd); }
		else if (strcmp(ins, "SEND") == 0)       { sendMsg(); }
		else if (strcmp(ins, "REGISTER") == 0)	{ registerError(); }
		else if (strcmp(ins, "CONNECT") == 0)    { connectTo(svfd); }
		else { Error("invalid instruction."); }
	}
	else { Error("invalid instruction."); }
}

/* =====================================================================
* ins is the instruction from server or other peer
* addrInfo cantains remote address info
* option: 0 -- run on server
		  1 -- run on client
* ====================================================================*/
void recveval(const char * ins, int fd, const int option)
{
	if (strcmp(ins, "CHECK") == 0)				{ checkValDes(fd); }
	else if (option){  // instructions only run on client
		if (strcmp(ins, "TERMINATERECV") == 0)       { receiveTmnt(fd); }
		else if (strcmp(ins, "MSG") == 0)       { receiveMsg(fd); }
		else if (strcmp(ins, "IPLIST") == 0)	{ displayIPList(); }
		else if (strcmp(ins, "VALID") == 0)    { Valid(); }
		else if (strcmp(ins, "INVALID") == 0)    { Invalid(); }
		else if (strcmp(ins, "SUCCESS") == 0)    { showSccss(fd); }
		else { Error("invalid instruction."); }
	}
	else { Error("invalid instruction."); }
}

/* =====================================================================
 * parse input string, return the first word as instruction
 * parameter stored in **para
 * ====================================================================*/
char * parse(char * input)
{
	char *instruction, *s = NULL;
	int i = 0;
	
	instruction = strtok_r(input, split, &s);  //for linux
	//instruction = strtok_s(input,split,&s);

	para[0] = strtok_r(NULL, split, &s);
	para[1] = s;  //for this project, the # of parameter is at most 2
	para[1][strlen(s) - 1] = '\0';

	/*while (para[0] != NULL){   
		i++;
		para[i] = strtok_r(NULL, split, &s); //for linux
		if (para[i] == NULL){
			free(para[i]);
			break;
		}
	}*/
	return instruction;
}

/* =====================================================================
 * parse recieve string, return the first word as instruction
 * raw message stored in *message
 * ====================================================================*/
char * recvparse(char * buf)
{
	char *instruction, *s = NULL;
	int i = 0;
	message = (char *)malloc(sizeof(char)* 500);
	//memset((void *)s, 0, sizeof(s));
	memset((void *)message, 0, sizeof(message));
	instruction = strtok_r(buf, split, &s);  //for linux
	//instruction = strtok_s(input,split,&s);
	strcpy(message,s);
	return instruction;
}

/* =====================================================================
 * client preparation
 * ====================================================================*/
void prepareClient(char * portNum)
{
	int pn = atoi(portNum);     //get int port number
	initialClient(pn);
}



/* =====================================================================
 * server preparation
 * ====================================================================*/
void prepareServer(char * portNum)
{
	int pn = atoi(portNum);
	initialServer(pn);
}

