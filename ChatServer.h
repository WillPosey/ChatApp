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
#include <mutex>
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

    void StartServer();

private:
    /* Methods used for setting up/tearing down server */
    void ListenForConnections();
    void ServerShutdown();
    void CloseAllSockets();

    /* Methods used in Establishing/Tearing Down Connection w/Client */
    bool InitializeConnection(int newSocket, string& newUsername);
    bool UsernameAvailable(string newUser);
    string CreateAllUsersMsg();
    void ClientThread(int clientSocket);
    void AddNewUser(string newUser, int socket_fd);
    void RemoveUser(string username);

    /* Methods to Send Information to Client(s) */
    void SendToClient(string destUser, string msg);
    void Broadcast(string sender, string msg);
    void Blockcast(string sender, string blockedUser, string msg);
    void NewConnection(string username);
    void Disconnection(string username);

    /* Methods used to receive and parse message */
    string ReceiveFromClient(int clientSocket, string username);
    string GetTag(string msg);
    bool CompareTag(string tag, string checkTag);
    string GetDestination(string msg);
    string GetFilename(string msg);
    string GetMsg(string msg);
    string GetFile(string msg);

    /* Method to synchronize displaying to terminal */
    void DisplayMsg(string msg);

    /* Server information */
    int listenSocket;
    bool setupSuccess;
    string serverPort;

    /* Locks for updating client information and displaying to terminal */
    mutex updateLock, displayLock;

    /* Client information */
    vector<string> usernames;
    map<string, int> clientSockets;
    map<int, pthread_t> clientThreadHandles;
};

/* Atmoic variable to signify that the signal handler */
/* function has run and SIGINT was the signal */
volatile sig_atomic_t signalDetected;

/* Signal Handler Function; used to shutdown server when ctrl+C entered at cmd line */
void signalHandler(int sigNum)
{
    signalDetected = (sigNum == SIGINT || sigNum == SIGABRT) ? 1 : 0;
}


#endif //CHAT_SERVER_H

