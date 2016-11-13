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
#include <algorithm>

/*******************************************************************
 *
 *      Main Method
 *
 *******************************************************************/
int main(int argc, char** argv)
{
    if(argc == 2)
    {
        ChatServer server (argv[1]);
        server.StartServer();
    }
    else
        cout << "Please specify server port" << endl;
    return 0;
}

/*******************************************************************
 *
 *      ChatServer Constructor
 *
 *******************************************************************/
ChatServer::ChatServer(char* port)
{
    serverPort = string(port);
    setupSuccess = false;
    signalDetected = 0;
    signal(SIGINT, signalHandler);
    listenSocket = -1;

    struct addrinfo hints, *results = NULL, *resPtr = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, port, &hints, &results);

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
    thread *initThread;         // thread to connect to client
    int connectSocket;          // socket for connected client
    struct timeval timeout;     // timeout for select() call
    fd_set fdSet;               // descriptor set for select()

    /* Listen for Connections */
    if(listen(listenSocket, MAX_LISTEN_QUEUE) < 0)
    {
        DisplayMsg("ERROR: Server Unable to Listen for Connections");
        return;
    }

    /*
     * Loop listening for connections using a select() call on the listening socket
     * If select() has a timeout, check if a signal was sent (SIGINT) to shutdown server
     * If listen socket has a connection waiting, accept the connection and create new thread
     */
    DisplayMsg("Chat Server Listening for Connections on Port: " + serverPort);
    while(1)
    {
        // these variables must be reset after every select() call
        FD_ZERO (&fdSet);
        FD_SET (listenSocket, &fdSet);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        select(listenSocket+1, &fdSet, NULL, NULL, &timeout);
        // signalDetected set by signal handler when SIGINT received
        if(signalDetected)
        {
            ServerShutdown();
            break;
        }
        // listenSocket is set when connection is waiting
        if(FD_ISSET (listenSocket, &fdSet))
        {
            connectSocket = accept(listenSocket, NULL, NULL);
            if(connectSocket < 0)
            {
                DisplayMsg("ERROR: An incoming connection attempt failed");
                continue;
            }
            initThread = new thread(&ChatServer::ClientThread, this, connectSocket);
            initThread->detach();
        }
    }
}

/*******************************************************************
 *
 *      ChatServer::ClientThread
 *
 *******************************************************************/
void ChatServer::ClientThread(int clientSocket)
{
    string username;
    if(!InitializeConnection(clientSocket, username))
        return;

    string recvMsg, sendMsg, destination, display, filename, data;
    string recvTag, sendTag;

    while(1)
    {
        recvMsg = ReceiveFromClient(clientSocket, username);
        recvTag = GetTag(recvMsg);

        // send message
        if(CompareTag(recvTag, SEND_MSG))
        {
            destination = GetDestination(recvMsg);
            if(destination.compare(username) == 0)
                continue;
            sendMsg = string(MSG_RCV) + USER_TAG + username + MSG_TAG + GetMsg(recvMsg) + MSG_TAG + MSG_END;
            DisplayMsg(username + " sent message to " + destination);
            SendToClient(destination, sendMsg);
            continue;
        }

        // send file
        if(CompareTag(recvTag, SEND_FILE))
        {
            destination = GetDestination(recvMsg);
            if(destination.compare(username) == 0)
                continue;
            sendMsg = string(FILE_RCV) + USER_TAG + username + MSG_TAG + GetFilename(recvMsg) + MSG_TAG + GetFile(recvMsg) + MSG_END;
            DisplayMsg(username + " sent file to " + destination);
            SendToClient(destination, sendMsg);
            continue;
        }

        // broadcast message
        if(CompareTag(recvTag, BRDCST_MSG))
        {
            sendMsg = string(MSG_RCV) + USER_TAG + username + MSG_TAG + GetMsg(recvMsg) + MSG_TAG + MSG_END;
            DisplayMsg(username + " broadcast message");
            Broadcast(username, sendMsg);
            continue;
        }

        // broadcast file
        if(CompareTag(recvTag, BRDCST_FILE))
        {
            sendMsg = string(FILE_RCV) + USER_TAG + username + MSG_TAG + GetFilename(recvMsg) + MSG_TAG + GetFile(recvMsg) + MSG_END;
            DisplayMsg(username + " broadcast file");
            Broadcast(username, sendMsg);
            continue;
        }

        // blockcast message
        if(CompareTag(recvTag, BLKCST_MSG))
        {
            destination = GetDestination(recvMsg);
            sendMsg = string(MSG_RCV) + USER_TAG + username + MSG_TAG + GetMsg(recvMsg) + MSG_TAG + MSG_END;
            DisplayMsg(username + " blockcast message, blocked " + destination);
            Blockcast(username, destination, sendMsg);
            continue;
        }

        // blockcast file
        if(CompareTag(recvTag, BLKCST_FILE))
        {
            destination = GetDestination(recvMsg);
            sendMsg = string(FILE_RCV) + USER_TAG + username + MSG_TAG + GetFilename(recvMsg) + MSG_TAG + GetFile(recvMsg) + MSG_END;
            DisplayMsg(username + " blockcast file, blocked " + destination);
            Blockcast(username, destination, sendMsg);
            continue;
        }

        // client shutdown
        if(CompareTag(recvTag, CLIENT_SHUTDOWN))
        {
            DisplayMsg(username + " disconnected");
            Disconnection(username);
            break;
        }
    }
}

/*******************************************************************
 *
 *      ChatServer::InitializeConnection
 *
 *******************************************************************/
bool ChatServer::InitializeConnection(int newSocket, string& newUsername)
{
    string nameRequest = string(USERNAME_REQUEST) + MSG_END;
    string otherUsers = CreateAllUsersMsg();
    string invalidName = string(USERNAME_TAKEN) + MSG_END;

    send(newSocket, nameRequest.c_str(), nameRequest.length(), 0);
    newUsername = ReceiveFromClient(newSocket, "connecting user");
    newUsername.pop_back();

    if(UsernameAvailable(newUsername))
    {
        send(newSocket, otherUsers.c_str(), otherUsers.length(), 0);
        NewConnection(newUsername);
        AddNewUser(newUsername, newSocket);
        DisplayMsg(newUsername + " connected");
        return true;
    }
    else
    {
        send(newSocket, invalidName.c_str(), invalidName.length(), 0);
        close(newSocket);
        return false;
    }
}

/*******************************************************************
 *
 *      ChatServer::ReceiveFromClient
 *
 *******************************************************************/
string ChatServer::ReceiveFromClient(int clientSocket, string username)
{
    string currentMsg;
    char msg[BUFFER_LENGTH];
    int numBytes;

    do
    {
        memset(msg, 0, BUFFER_LENGTH);
        numBytes = recv(clientSocket, msg, BUFFER_LENGTH, 0);
        if(numBytes < 0)
        {
            DisplayMsg("ERROR: could not receive from [" + username + "]");
            return "";
        }
        currentMsg += string(msg, numBytes);
    } while(msg[numBytes-1] != MSG_END);

    return currentMsg;
}

/*******************************************************************
 *
 *      ChatServer::SendToClient
 *
 *******************************************************************/
void ChatServer::SendToClient(string username, string msg)
{
    updateLock.lock();
    int socket_fd = clientSockets[username];
    updateLock.unlock();

    send(socket_fd, msg.c_str(), msg.length(), 0);
}

/*******************************************************************
 *
 *      ChatServer::Broadcast
 *
 *******************************************************************/
void ChatServer::Broadcast(string sender, string msg)
{
    vector<string>::iterator it;
    vector<string>tempUsernames;

    updateLock.lock();
    tempUsernames = usernames;
    updateLock.unlock();

    for(it = tempUsernames.begin(); it != tempUsernames.end(); it++)
        if(sender.compare(*it) != 0)
            SendToClient(*it, msg);
}

/*******************************************************************
 *
 *      ChatServer::Blockcast
 *
 *******************************************************************/
void ChatServer::Blockcast(string sender, string blockedUser, string msg)
{
    vector<string>::iterator it;
    vector<string>tempUsernames;

    updateLock.lock();
    tempUsernames = usernames;
    updateLock.unlock();

    for(it = tempUsernames.begin(); it != tempUsernames.end(); it++)
        if(blockedUser.compare(*it) != 0 && sender.compare(*it) != 0)
            SendToClient(*it, msg);
}

/*******************************************************************
 *
 *      ChatServer::NewConnection
 *
 *******************************************************************/
void ChatServer::NewConnection(string username)
{
    string connectionMsg = string(USER_CONNECT) + USER_TAG + username + MSG_END;
    Broadcast(username, connectionMsg);
}

/*******************************************************************
 *
 *      ChatServer::Disconnection
 *
 *******************************************************************/
void ChatServer::Disconnection(string username)
{
    string disconnectMsg = string(USER_DISCONNECT) + USER_TAG + username + MSG_END;
    RemoveUser(username);
    Broadcast(username, disconnectMsg);
}

/*******************************************************************
 *
 *      ChatServer::ServerShutdown
 *
 *******************************************************************/
void ChatServer::ServerShutdown()
{
    close(listenSocket);
    DisplayMsg("\nServer Shutting Down, Notifying Clients...");
    string msg = string(SERVER_SHUTDOWN) + MSG_END;
    Broadcast(" ", msg);
    CloseAllSockets();
}

/*******************************************************************
 *
 *      ChatServer::UsernameAvailable
 *
 *******************************************************************/
bool ChatServer::UsernameAvailable(string newUser)
{
    bool available = true;

    updateLock.lock();
    if(find(usernames.begin(), usernames.end(), newUser) != usernames.end())
        available = false;
    updateLock.unlock();

    return available;
}

/*******************************************************************
 *
 *      ChatServer::AddNewUser
 *
 *******************************************************************/
void ChatServer::AddNewUser(string newUser, int socket_fd)
{
    updateLock.lock();
    usernames.push_back(newUser);
    clientSockets.insert(pair<string,int>(newUser, socket_fd));
    updateLock.unlock();
}

/*******************************************************************
 *
 *      ChatServer::RemoveUser
 *
 *******************************************************************/
void ChatServer::RemoveUser(string username)
{
    vector<string>::iterator usernameIt;
    map<string,int>::iterator socketIt;

    updateLock.lock();
    usernameIt = find(usernames.begin(), usernames.end(), username);
    usernames.erase(usernameIt);
    socketIt = clientSockets.find(username);
    clientSockets.erase(socketIt);
    updateLock.unlock();
}

/*******************************************************************
 *
 *      ChatServer::CreateAllUsersMsg
 *
 *******************************************************************/
string ChatServer::CreateAllUsersMsg()
{
    string allUsers = string(OTHER_USERS);
    vector<string>::iterator userIterator;

    updateLock.lock();
    for(userIterator = usernames.begin(); userIterator != usernames.end(); userIterator++)
    {
        allUsers += USER_TAG;
        allUsers += *userIterator;
    }
    allUsers += MSG_END;
    updateLock.unlock();

    return allUsers;
}

/*******************************************************************
 *
 *      ChatServer::DisplayMsg
 *
 *******************************************************************/
void ChatServer::DisplayMsg(string msg)
{
    displayLock.lock();
    cout << msg << endl;
    displayLock.unlock();
}

/*******************************************************************
 *
 *      ChatServer::GetTag
 *
 *******************************************************************/
string ChatServer::GetTag(string msg)
{
    return msg.substr(0,2);
}

/*******************************************************************
 *
 *      ChatServer::CompareTag
 *
 *******************************************************************/
bool ChatServer::CompareTag(string tag, string checkTag)
{
    return (tag.compare(checkTag) == 0);
}

/*******************************************************************
 *
 *      ChatServer::GetDestination
 *
 *******************************************************************/
string ChatServer::GetDestination(string msg)
{
    int start = msg.find(USER_TAG)+1;
    int finish = msg.find(MSG_TAG);
    return msg.substr(start, finish-start);
}

/*******************************************************************
 *
 *      ChatServer::GetFilename
 *
 *******************************************************************/
string ChatServer::GetFilename(string msg)
{
    int start = msg.find(MSG_TAG)+1;
    int finish = msg.find(MSG_TAG, start);
    return msg.substr(start, finish-start);
}

/*******************************************************************
 *
 *      ChatServer::GetMsg
 *
 *******************************************************************/
string ChatServer::GetMsg(string msg)
{
    int start = msg.find(MSG_TAG)+1;
    int finish = msg.find(MSG_TAG, start);
    return msg.substr(start, finish-start);
}

/*******************************************************************
 *
 *      ChatServer::GetFile
 *
 *******************************************************************/
string ChatServer::GetFile(string msg)
{
    int start = msg.find(MSG_TAG)+1;
    start = msg.find(MSG_TAG, start)+1;
    return msg.substr(start, msg.length()-start-1);
}

/*******************************************************************
 *
 *      ChatServer::CloseAllSockets
 *
 *******************************************************************/
void ChatServer::CloseAllSockets()
{
    vector<string>::iterator it;

    for(it = usernames.begin(); it != usernames.end(); it++)
        close(clientSockets[*it]);
}

