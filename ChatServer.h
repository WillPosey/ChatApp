/*******************************************************************
 *      ChatServer.h
 *
 *      Author: William Posey
 *      Course: CNT5106C
 *
 *      Header file for Server in Internet Chat Application
 *******************************************************************/
#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <vector>
#include <map>
#include <thread>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#define MAX_LISTEN_QUEUE        10
#define MAX_TRIES               10
#define BUFFER_LENGTH           100

using namespace std;

class ChatServer
{
public:
    ChatServer(char* port);
    ~ChatServer();

    void StartServer();
    int GetSockets(int* sockets);

private:
    void ListenForConnections();
    void CreateThread(int newSocket);
    bool InitializeConnection(int newSocket, string& newUsername);
    void CloseAllSockets();

    void ClientThread(int clientSocket, string username);
    string ReceiveFromClient(int clientSocket, string username);

    void SendToClient(string username, string msg);
    void Broadcast(string sender, string msg);
    void Blockcast(string sender, string username, string msg);
    void NewConnection(string username);
    void Disconnection(string username);

    bool UsernameAvailable(string newUser);
    void AddNewUser(string newUser, int socket_fd);
    void RemoveUser(string username);
    string CreateAllUsersMsg();

    char   GetMsgTag(string msg);
    string GetMsgDestination(string msg);
    string GetBlockedDestination(string msg);
    string GetMsgFilename(string msg, char tag);
    string GetMsgData(string msg, char tag);
    string RemoveMsgDestination(string msg);

    void DisplayMsg(string msg);

    int listenSocket;
    bool setupSuccess;
    string serverPort;

    pthread_mutex_t updateLock, displayLock;

    vector<string> usernames;
    map<string, int> clientSockets;
};


volatile sig_atomic_t signalDetected;

void signalHandler(int sigNum)
{
    signalDetected = (sigNum == SIGINT) ? 1 : 0;
}


#endif //CHAT_SERVER_H

