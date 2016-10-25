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

private:
    void ListenForConnections();
    void CreateThread(int newSocket);
    bool InitializeConnection(int newSocket, string& newUsername);
    void CloseAllSockets();

    void ClientThread(int clientSocket);
    string ReceiveFromClient(int clientSocket, string username);

    void SendToClient(string destUser, string msg);
    void Broadcast(string sender, string msg);
    void Blockcast(string sender, string blockedUser, string msg);
    void NewConnection(string username);
    void Disconnection(string username);
    void ServerShutdown();

    bool UsernameAvailable(string newUser);
    void AddNewUser(string newUser, int socket_fd);
    void RemoveUser(string username);
    string CreateAllUsersMsg();

    string GetTag(string msg);
    string GetDestination(string msg);
    string GetFilename(string msg);
    string GetMsg(string msg);
    string GetFile(string msg);
    bool CompareTag(string tag, string checkTag);

    void DisplayMsg(string msg);

    int listenSocket;
    bool setupSuccess;
    string serverPort;

    pthread_mutex_t updateLock, displayLock;

    vector<string> usernames;
    map<string, int> clientSockets;
    map<int, pthread_t> clientThreadHandles;
};


volatile sig_atomic_t signalDetected;

void signalHandler(int sigNum)
{
    signalDetected = (sigNum == SIGINT) ? 1 : 0;
}


#endif //CHAT_SERVER_H

