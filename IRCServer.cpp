
const char * usage =
"                                                               \n"
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <map>
#include <fstream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdio.h>

#include "IRCServer.h"
using namespace std;

int QueueLength = 5;
int totalRooms = 0;
map<string, vector<string>> users;
map<string, vector<string>> mess;
fstream passFile;

vector<string> usernames;
vector<string> passwords;
vector<string> rooms;

//test

int
IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);
  
	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			     (char *) &optval, sizeof( int ) );
	
	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			  (struct sockaddr *)&serverIPAddress,
			  sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}
	
	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

void
IRCServer::runServer(int port)
{
	int masterSocket = open_server_socket(port);

	initialize();
	
	while ( 1 ) {
		
		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket = accept( masterSocket,
					  (struct sockaddr *)&clientIPAddress,
					  (socklen_t*)&alen);
		
		if ( slaveSocket < 0 ) {
			perror( "accept" );
			exit( -1 );
		}
		
		// Process request.
		processRequest( slaveSocket );		
	}
}

int
main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}
	
	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);
	
}

//
// Commands:
//   Commands are started y the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

void
IRCServer::processRequest( int fd )
{
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;
	
	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;
	
	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while ( commandLineLength < MaxCommandLine &&
		read( fd, &newChar, 1) > 0 ) {

		if (newChar == '\n' && prevChar == '\r') {
			break;
		}
		
		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}
	
	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
        commandLine[ commandLineLength ] = 0;

	printf("RECEIVED: %s\n", commandLine);

	printf("The commandLine has the following format:\n");
	printf("COMMAND <user> <password> <arguments>. See below.\n");
	printf("You need to separate the commandLine into those components\n");
	printf("For now, command, user, and password are hardwired.\n");
	
/*	std::string s1 = std::string(commandLine);
	std::string command1;
	std::string user1;
	std::string password1;
	std::string args1;
	if(s1.find(" ") != std::string::npos) {
		command1 = s1.substr(0, s1.find(" "));
		s1 = s1.substr(s1.find(" ") + 1);
	}
	if(s1.find(" ") != std::string::npos) {
		user1 = s1.substr(0, s1.find(" "));
		s1 = s1.substr(s1.find(" ") + 1);
	}
	if(s1.find(" ") != std::string::npos) {
		password1 = s1.substr(0, s1.find(" "));
		s1 = s1.substr(s1.find(" ") + 1);
		if(s1.find(" ") != std::string::npos) {
			args1 = "";
		} else {
			args1 = s1;
		}
	} else {
		password1 = s1;
	}
*/
	char command1[1025];
	char user1[1025];
	char password1[1025];
	char args1[1025];
	memset(command1, 0, 1025);
	memset(user1, 0, 1025);
	memset(password1, 0, 1025);
	memset(args1, 0, 1025);
	int nRead = sscanf(commandLine, "%s %s %s %[^\n]", command1, user1, password1, args1);

	const char * command = command1;
	const char * user = user1;
	const char * password = password1;
	const char * args = args1;
	printf("command=%s\n", command);
	printf("user=%s\n", user);
	printf( "password=%s\n", password );
	printf("args=%s\n", args);

	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password, args);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password, args);
	}
	else if (!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LIST-ROOMS")) {
		listRooms(fd, user, password, args);
	}
	else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	// Send OK answer
	//const char * msg =  "OK\n";
	//write(fd, msg, strlen(msg));

	close(fd);	
}

void
IRCServer::initialize()
{
	// Open password file
	passFile.open("password.txt");
	string s1;
	int i = 1;
	if(passFile.is_open()) {
		while(getline(passFile, s1)) {
			if((i % 3 == 1)) { //skip every third line format: username\npassword\n\n
				//Initialize usernames
				usernames.push_back(s1);
				i++;
				getline(passFile, s1);
				passwords.push_back(s1);
				i++;
			} else {
				i++;
			}
		}
		passFile.close();
	}
	
	// Initialize users in room

	// Initalize message list

}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {
	// Here check the password
	for(int i = 0; i < passwords.size(); i++) {
		if(!(passwords[i].compare(password)) && !(usernames[i].compare(user))) {
			return true;
		}
	}
	return false;
}

void
IRCServer::addUser(int fd, const char * user, const char * password, const char * args)
{
	// Here add a new user. For now always return OK.
	int found = 0;
	for(int i = 0; i < usernames.size(); i++) {
		if(!(usernames[i].compare(user))) {
			found = 1;
		}
	}
	if(found) {
		const char * message = "DENIED\r\n";
		write(fd, message, strlen(message));
	} else {
		passFile.open("password.txt");
		passFile << user << "\n" << password << "\n\n";
		passFile.close();
		usernames.push_back(user);
		passwords.push_back(password);
		const char * message = "OK\r\n";
		write(fd, message, strlen(message));
	}
	return;		
}

void
IRCServer::createRoom(int fd, const char * user, const char * password, const char * args) {
	if(checkPassword(fd, user, password)) {
		int roomFound = 0;
		for(int i = 0; i < rooms.size(); i++) {
			if(!(rooms[i].compare(args))) {
				roomFound = 1;
			}
		}
		if(!roomFound) {
			rooms.push_back(args);
			totalRooms++;
			const char * message = "OK\r\n";
			write(fd, message, strlen(message));
		} else {
			const char * message = "DENIED\r\n";
			write(fd, message, strlen(message));
		}
	} else {
		const char * message = "ERROR (Wrong password)\r\n";
		write(fd, message, strlen(message));
	}
	return;
}
void
IRCServer::listRooms(int fd, const char * user, const char * password, const char * args) {
	if(checkPassword(fd, user, password)) {
		vector<string> temp(rooms);
		sort(temp.begin(), temp.end());
		for(int i = 0; i < rooms.size(); i++) {
			const char * message = temp[i].c_str();
			write(fd, message, strlen(message));
			const char * msg = "\r\n";
			write(fd, msg, strlen(msg));
		}
	} else { 
		const char * message = "ERROR (Wrong password)\r\n";
		write(fd, message, strlen(message));
	}
	return;
}

int loopUsers(vector<string> s1, string s2) {
	for(int i = 0; i < s1.size(); i++) {
		if(!(s1[i].compare(s2))) {
			return 1; //FOUND
		}
	}
	return 0; //No match
}

void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		int roomFound = 0;
		for(int i = 0; i < rooms.size(); i++) {
			if(!(rooms[i].compare(args))) {
				roomFound = 1;
			}
		}
		if(!roomFound) {
			const char * message = "ERROR (No room)\r\n";
			write(fd, message, strlen(message));
		} else {
			if(users.find(user) == users.end() || !loopUsers(users[user], args)) {
				vector<string> v1;
				users.insert(make_pair(user, v1));
				users[user].push_back(args);
				const char * msg = "OK\r\n";
				write(fd, msg, strlen(msg));
			} else {
				const char * msg = "OK\r\n";
				write(fd, msg, strlen(msg));
			}
		}
	} else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}
}

void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
	if(checkPassword(fd, user, password)) {
		if(users.find(user) != users.end()) {
			int found = 0;
			for(int i = 0; i < users[user].size(); i++) {
				if(!(users[user][i].compare(args))) {
					users[user].erase(users[user].begin() + i);
					const char * msg = "OK\r\n";
					write(fd, msg, strlen(msg));
					found = 1;
				}
			}
			if(!found) {
				const char * msg = "DENIED\r\n";
				write(fd, msg, strlen(msg));
			}
		} else {
			const char * msg = "ERROR (No user in room)\r\n";
			write(fd, msg, strlen(msg));
		}
	} else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}	
}

void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
	char roomName[100];
	char message[1025];
	int n = sscanf(args, "%s %[^\n]", roomName, message);
	string room = roomName;
	string user2 = user;
	if(checkPassword(fd, user, password)) {
		if(users.find(user) != users.end() && loopUsers(users[user], room)) { 
			if(!(mess.find(room) != mess.end())) {
				string s1 = "0 " + user2 + " " + message + "\r\n";
				vector <string> v1;
				mess.insert(make_pair(room, v1));
				mess[room].push_back(s1);
			} else {
				int j = mess[room].size();
				string s1 = to_string(j) + " " + user2 + " " + message + "\r\n";
				mess[room].push_back(s1);
			}
			const char * msg = "OK\r\n";
			write(fd, msg, strlen(msg));
		} else {
			const char * msg = "ERROR (user not in room)\r\n";
			write(fd, msg, strlen(msg));
		}
	} else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}
}

void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{
	int last;
	char roomName[100];
	int n = sscanf(args, "%d %[^\n]", &last, roomName);
	if(checkPassword(fd, user, password)) {
		if(users.find(user) != users.end() && loopUsers(users[user], roomName)) {
			//int i = mess[roomName].size() - 100;
			//if(0 > i) {
			//	i = 0;
			//}
			if(last + 1 >= mess[roomName].size()) {
				const char * msg = "NO-NEW-MESSAGES\r\n";
				write(fd, msg, strlen(msg));
			} else {
				for(int i = last + 1; i < mess[roomName].size(); i++) {
					const char * msg = mess[roomName][i].c_str();
					write(fd, msg, strlen(msg));
				}
				const char * msg = "\r\n";
				write(fd, msg, strlen(msg));
			}
		} else {
			const char * msg = "ERROR (User not in room)\r\n";
			write(fd, msg, strlen(msg));
		}
	} else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}
}

void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{	
	string s1;
	if(checkPassword(fd, user, password)) {
		map<string, vector <string>>::iterator it;
		for(it = users.begin(); it != users.end(); it++) {
			vector <string> v1 = it->second;
			for(int i = 0; i < v1.size(); i++) {
				if(!(v1[i].compare(args))) {
					s1 += it->first + "\r\n";
				}
			}
		}
		const char * msg = s1.c_str();
		write(fd, msg, strlen(msg));
		const char * msg2 = "\r\n";
		write(fd, msg2, strlen(msg2));
	} else {
		const char * msg = "ERROR (Wrong password)\r\n";
		write(fd, msg, strlen(msg));
	}
}

void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
	if(checkPassword(fd, user, password)) {
		vector<string> temp (usernames);
		sort(temp.begin(), temp.end());
		for(int i = 0; i < temp.size(); i++) {
			const char * message = temp[i].c_str();
			write(fd, message, strlen(message));
			const char * msg = "\r\n";
			write(fd, msg, strlen(msg));
		}
		const char * msg = "\r\n";
		write(fd, msg, strlen(msg));
	} else {
		const char * message = "ERROR (Wrong password)\r\n";
		write(fd, message, strlen(message));
	}
}

