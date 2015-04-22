A simple server/clients chatting program, working as a interactive interpreter.

Start program with <program name> <arg1> <arg2>
arg1: "s" works as a server, "c" works as a client
arg2: port number, ex. "5050"

It implements commands and functionality listed below:
	-MYIP	Obtain the IP address.
	-MYPORT  The port number the socket is listening to.
	-REGISTER <SERVER_IP> <PORT_NO>  Register the client with server.
	-CONNECT <DESTINATION> <PORT_NO>  Establish connection between two clients.
	-LIST  Display all connection of this process.
	-TERMINATE <CONNECTION_ID>  Terminate the connection under its id.
	-EXIT  Close all connections and terminate the process.
	-SEND <CONNECTION_ID> <MSG>  Send a message to the host under its id.