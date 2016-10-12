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
    void ListenForConnections();
    void CreateThread(int newSocket);
    bool InitializeConnection(int newSocket, string& newUsername);

    void ClientThread(int clientSocket, string username);
    string ReceiveFromClient(int clientSocket, string username);

    void SendToClient(string username, string msg);
    void Broadcast(string msg);
    void Blockcast(string username, string msg);
    void NewConnection(string username);
    void Disconnection(string username);

    bool UsernameAvailable(string newUser);
    void AddNewUser(string newUser, int socket_fd);
    void RemoveUser(string username);
    string CreateAllUsersMsg();

    char   GetMsgTag(string msg);
    string GetMsgSource(string msg);
    string GetMsgDestination(string msg);
    string GetBlockedDestination(string msg);
    string GetMsgFilename(string msg);
    string GetMsgData(string msg);

    void DisplayMsg(string msg);

private:
    int listenSocket;
    bool setupSuccess;
    string serverPort;

    pthread_mutex_t updateLock, displayLock;

    vector<string> usernames;
    map<string, int> clientSockets;
};

#endif //CHAT_SERVER_H

