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

#define MAX_LISTEN_QUEUE 10

using namespace std;

class ChatServer
{
public:
    ChatServer();
    ~ChatServer();

    void StartServer();
    void ListenForConnections();
    void CreateThread();

    void ClientThread();
    void InitializeConnection();
    void ReceiveFromClient();

    void SendToClient(string username, string msg);
    void Broadcast(string msg);
    void Blockcast(string username, string msg);

private:
    int listenSocket;
    bool setupSuccess;
    int serverPort;

    pthread_mutex_t updateLock;

    vector<string> usernames;
    map<string, int> clientSockets;
    map<string, thread> clientThreads;
};

#endif //CHAT_SERVER_H

