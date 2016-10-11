/*******************************************************************
 *      ChatServer.cpp
 *
 *      Author: William Posey
 *      Course: CNT5106C
 *
 *******************************************************************/
#include "MessageDefs.h"
#include "ChatServer.h"
#include <cstring>

/*******************************************************************
 *
 *      Main Method
 *
 *******************************************************************/
int main()
{
    ChatServer server;
    server.StartServer();
    return 0;
}

/*******************************************************************
 *
 *      ChatServer Constructor
 *
 *******************************************************************/
ChatServer::ChatServer()
{
    setupSuccess = false;
    listenSocket = -1;
    pthread_mutex_init(&updateLock, NULL);

    struct addrinfo hints, *results = NULL, *resPtr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, "0", &hints, &results);

    for(resPtr=results; resPtr!=NULL; resPtr=resPtr->ai_next)
    {
        listenSocket = socket(resPtr->ai_family, resPtr->ai_socktype, resPtr->ai_protocol);
        if (listenSocket < 0)
            continue;
        if (bind(listenSocket, resPtr->ai_addr, resPtr->ai_addrlen) < 0)
        {
            close(listenSocket);
            continue;
        }
        break;
    }

    setupSuccess = (resPtr!=NULL);

    if(setupSuccess)
    {
        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in addr = *(sockaddr_in*)resPtr->ai_addr;
        struct sockaddr assignedSock;
        struct sockaddr_in *sock_ptr = (struct sockaddr_in *) &assignedSock;
        socklen_t sizeBuf = sizeof(struct sockaddr);

        serverIP = string(inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN));
        getsockname(listenSocket, &assignedSock, &sizeBuf);
        serverPort = ntohs(sock_ptr->sin_port);
    }

    freeaddrinfo(results);
}

/*******************************************************************
 *
 *      ChatServer Destructor
 *
 *******************************************************************/
ChatServer::~ChatServer()
{
    listenSocket = -1;
    pthread_mutex_destroy(&updateLock);
}

/*******************************************************************
 *
 *      ChatServer::StartServer
 *
 *******************************************************************/
void ChatServer::StartServer()
{
    if(!setupSuccess)
    {
        cout << "ERROR: Server Could Not Bind to Listen Socket" << endl;
        return;
    }
    ListenForConnections();
}

/*******************************************************************
 *
 *      ChatServer::ListenForConnections
 *
 *******************************************************************/
void ChatServer::ListenForConnections()
{
    if(listen(listenSocket, MAX_LISTEN_QUEUE) < 0)
    {
        cout << "ERROR: Server Unable to Listen for Connections" << endl;
        return;
    }

    int tempSocket, *newSocket;

    cout << "IP[" << serverIP << "] Port[" << serverPort << "]" << endl;
    cout << "Chat Server Listening for Connections..." << endl;

    //while(1)
    //{

    //}
}


