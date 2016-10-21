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
    ChatServer server (argv[1]);
    server.StartServer();
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
    pthread_mutex_init(&updateLock, NULL);
    pthread_mutex_init(&displayLock, NULL);

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
    pthread_mutex_destroy(&updateLock);
    pthread_mutex_destroy(&displayLock);
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
    thread *initThread;
    if(listen(listenSocket, MAX_LISTEN_QUEUE) < 0)
    {
        cout << "ERROR: Server Unable to Listen for Connections" << endl;
        return;
    }
    cout << "Chat Server Listening for Connections on Port <" << serverPort << ">" << endl;

    int connectSocket;
    struct timeval timeout;
    fd_set fdSet;

    while(1)
    {
        FD_ZERO (&fdSet);
        FD_SET (listenSocket, &fdSet);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        select(listenSocket+1, &fdSet, NULL, NULL, &timeout);
        if(signalDetected)
        {
            close(listenSocket);
            DisplayMsg("\nServer Shutting Down, Notifying Clients...");
            char tag = SERVER_SHUTDOWN, endMsg = MSG_END;
            string msg = "";
            msg += tag + endMsg;
            Broadcast(msg);
            CloseAllSockets();
            break;
        }
        if(FD_ISSET (listenSocket, &fdSet))
        {
            connectSocket = accept(listenSocket, NULL, NULL);
            if(connectSocket < 0)
            {
                DisplayMsg("ERROR: An incoming connection attempt failed");
                continue;
            }
            initThread = new thread(&ChatServer::CreateThread, this, connectSocket);
            initThread->detach();
        }
    }
}

/*******************************************************************
 *
 *      ChatServer::CreateThread
 *
 *******************************************************************/
void ChatServer::CreateThread(int newSocket)
{
    string newUser;
    if(!InitializeConnection(newSocket, newUser))
        return;
    thread clientThread (&ChatServer::ClientThread, this, newSocket, newUser);
    clientThread.detach();
}

/*******************************************************************
 *
 *      ChatServer::InitializeConnection
 *
 *******************************************************************/
bool ChatServer::InitializeConnection(int newSocket, string& newUsername)
{
    int tries = 0, numBytes;
    bool sent = false, usernameRecvErr = false, usernameAccepted = false;
    char usernameReq = USERNAME_REQUEST, usernameValid = USERNAME_VALID;
    char newUsernameBuf[BUFFER_LENGTH];
    char otherUsersReqBuf;
    string otherUsers, temp;

    while(!usernameAccepted)
    {
        /* Send Client Username Request */
        while(!sent)
        {
            if(send(newSocket, &usernameReq, 1, 0) < 0)
            {
                DisplayMsg("ERROR: could not send username request to new client");
                if(tries++ == MAX_TRIES)
                {
                    DisplayMsg("Dropping new connection; could not get username");
                    close(newSocket);
                    break;
                }
            }
            else
                sent = true;
        }
        if(!sent)
            return false;

        string newUsername = "";

        /* Retrieve Client's Username */
        do
        {
            memset(newUsernameBuf, 0, BUFFER_LENGTH);
            numBytes = recv(newSocket, newUsernameBuf, BUFFER_LENGTH, 0);
            if(numBytes < 0)
            {
                DisplayMsg("ERROR: could not receive new username");
                usernameRecvErr = true;
                break;
            }
            newUsername += string(newUsernameBuf);
        } while(newUsernameBuf[numBytes-1] != MSG_END);

        if(usernameRecvErr || newUsername[0] != USERNAME_SUBMIT)
            return false;

        /* Check if username available */
        newUsername.erase(0,1);  //remove USERNAME_SUBMIT code
        newUsername.pop_back();   //remove END_MSG indicator
        if(UsernameAvailable(newUsername))
        {
            DisplayMsg("[" + newUsername + "]->CONNECTED");
            otherUsers = CreateAllUsersMsg();
            AddNewUser(newUsername, newSocket);
            usernameAccepted = true;
            send(newSocket, &usernameValid, 1, 0);
            temp = newUsername;
        }
        else
        {
            sent = false;
            DisplayMsg("New Client Attempted Connection; Username [" + newUsername + "] Already In Use");
        }
    }
    newUsername = temp;
    recv(newSocket, &otherUsersReqBuf, 1, 0);
    send(newSocket, otherUsers.c_str(), otherUsers.length(), 0);
    NewConnection(newUsername);
    return true;
}

/*******************************************************************
 *
 *      ChatServer::ClientThread
 *
 *******************************************************************/
void ChatServer::ClientThread(int clientSocket, string username)
{
    string recvMsg, sendMsg, source, destination, display, filename, data;
    char recvTag, sendTag;

    source = "[" + username + "]->";

    while(1)
    {
        recvMsg = ReceiveFromClient(clientSocket, username);
        recvTag = GetMsgTag(recvMsg);
        switch(recvTag)
        {
            case SEND_MSG:
                sendTag = MSG_RCV;
                destination = GetMsgDestination(recvMsg);
                if(destination.compare(username) == 0)
                    break;
                data = GetMsgData(recvMsg, recvTag);
                sendMsg = RemoveMsgDestination(recvMsg).replace(0,1,1,sendTag);
                DisplayMsg(source + "[" + destination + "]: " + data);
                SendToClient(destination, sendMsg);
                break;
            case SEND_FILE:
                sendTag = FILE_RCV;
                destination = GetMsgDestination(recvMsg);
                if(destination.compare(username) == 0)
                    break;
                filename = GetMsgFilename(recvMsg, recvTag);
                sendMsg = RemoveMsgDestination(recvMsg).replace(0,1,1,sendTag);
                DisplayMsg(source + "[" + destination + "]: " + filename);
                SendToClient(destination, sendMsg);
                break;
            case BRDCST_MSG:
                sendTag = MSG_RCV;
                data = GetMsgData(recvMsg, recvTag);
                sendMsg = recvMsg.replace(0,1,1,sendTag);
                DisplayMsg(source + " <BROADCAST>: " + data);
                Blockcast(username, sendMsg);
                break;
            case BRDCST_FILE:
                sendTag = FILE_RCV;
                filename = GetMsgFilename(recvMsg, recvTag);
                sendMsg = recvMsg.replace(0,1,1,sendTag);
                DisplayMsg(source + " <BROADCAST>: " + filename);
                Blockcast(username, sendMsg);
                break;
            case BLKCST_MSG:
                sendTag = MSG_RCV;
                destination = GetBlockedDestination(recvMsg);
                data = GetMsgData(recvMsg, recvTag);
                sendMsg = RemoveMsgDestination(recvMsg).replace(0,1,1,sendTag);
                DisplayMsg(source + " <BLOCKCAST>[" + destination + "]: " + data);
                Blockcast(destination, sendMsg);
                break;
            case BLKCST_FILE:
                sendTag = FILE_RCV;
                destination = GetBlockedDestination(recvMsg);
                filename = GetMsgFilename(recvMsg, recvTag);
                sendMsg = RemoveMsgDestination(recvMsg).replace(0,1,1,sendTag);
                DisplayMsg(source + " <BLOCKCAST>[" + destination + "]: " + filename);
                Blockcast(destination, sendMsg);
                break;
            case CLIENT_SHUTDOWN:
                DisplayMsg(source + "DISCONNECTED");
                Disconnection(username);
                break;
            default:
                break;
        }
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
        currentMsg += string(msg);
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
    pthread_mutex_lock(&updateLock);
    int socket_fd = clientSockets[username];
    pthread_mutex_unlock(&updateLock);
    send(socket_fd, msg.c_str(), msg.length(), 0);
}

/*******************************************************************
 *
 *      ChatServer::Broadcast
 *
 *******************************************************************/
void ChatServer::Broadcast(string msg)
{
    vector<string>::iterator it;
    vector<string>tempUsernames;

    pthread_mutex_lock(&updateLock);
    tempUsernames = usernames;
    pthread_mutex_unlock(&updateLock);

    for(it = tempUsernames.begin(); it != tempUsernames.end(); it++)
        SendToClient(*it, msg);
}

/*******************************************************************
 *
 *      ChatServer::Blockcast
 *
 *******************************************************************/
void ChatServer::Blockcast(string username, string msg)
{
    vector<string>::iterator it;
    vector<string>tempUsernames;

    pthread_mutex_lock(&updateLock);
    tempUsernames = usernames;
    pthread_mutex_unlock(&updateLock);

    for(it = tempUsernames.begin(); it != tempUsernames.end(); it++)
        if(username.compare(*it) != 0)
            SendToClient(*it, msg);
}

/*******************************************************************
 *
 *      ChatServer::NewConnection
 *
 *******************************************************************/
void ChatServer::NewConnection(string username)
{
    string connectionMsg;
    char tag = USER_CONNECT;
    connectionMsg = tag + username + MSG_END;
    Blockcast(username, connectionMsg);
}

/*******************************************************************
 *
 *      ChatServer::Disconnection
 *
 *******************************************************************/
void ChatServer::Disconnection(string username)
{
    string disconnectMsg;
    char tag = USER_DISCONNECT;
    disconnectMsg = tag + username + MSG_END;
    RemoveUser(username);
    Broadcast(disconnectMsg);
}

/*******************************************************************
 *
 *      ChatServer::UsernameAvailable
 *
 *******************************************************************/
bool ChatServer::UsernameAvailable(string newUser)
{
    bool available = true;
    pthread_mutex_lock(&updateLock);
    if(find(usernames.begin(), usernames.end(), newUser) != usernames.end())
        available = false;
    pthread_mutex_unlock(&updateLock);
    return available;
}

/*******************************************************************
 *
 *      ChatServer::AddNewUser
 *
 *******************************************************************/
void ChatServer::AddNewUser(string newUser, int socket_fd)
{
    pthread_mutex_lock(&updateLock);
    usernames.push_back(newUser);
    clientSockets.insert(pair<string,int>(newUser, socket_fd));
    pthread_mutex_unlock(&updateLock);
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

    pthread_mutex_lock(&updateLock);
    usernameIt = find(usernames.begin(), usernames.end(), username);
    usernames.erase(usernameIt);
    socketIt = clientSockets.find(username);
    clientSockets.erase(socketIt);
    pthread_mutex_unlock(&updateLock);
}

/*******************************************************************
 *
 *      ChatServer::CreateAllUsersMsg
 *
 *******************************************************************/
string ChatServer::CreateAllUsersMsg()
{
    char reply = USERNAMES_REPLY;
    string allUsers = "";
    allUsers += reply;
    vector<string>::iterator userIterator;
    pthread_mutex_lock(&updateLock);
    for(userIterator = usernames.begin(); userIterator != usernames.end(); userIterator++)
    {
        allUsers += *userIterator;
        allUsers += USERNAME_END;
    }
    allUsers += MSG_END;
    pthread_mutex_unlock(&updateLock);
    return allUsers;
}

/*******************************************************************
 *
 *      ChatServer::DisplayMsg
 *
 *******************************************************************/
void ChatServer::DisplayMsg(string msg)
{
    pthread_mutex_lock(&displayLock);
    cout << msg << endl;
    pthread_mutex_unlock(&displayLock);
}

/*******************************************************************
 *
 *      ChatServer::GetMsgTag
 *
 *******************************************************************/
char ChatServer::GetMsgTag(string msg)
{
    return msg[0];
}

/*******************************************************************
 *
 *      ChatServer::GetMsgDestination
 *
 *******************************************************************/
string ChatServer::GetMsgDestination(string msg)
{
    int start = msg.find(MSG_SRC_END)+1;
    int finish = msg.find(MSG_DST_END);
    return msg.substr(start, finish-start);

}

/*******************************************************************
 *
 *      ChatServer::GetBlockedDestination
 *
 *******************************************************************/
string ChatServer::GetBlockedDestination(string msg)
{
    return GetMsgDestination(msg);
}

/*******************************************************************
 *
 *      ChatServer::RemoveMsgDestination
 *
 *******************************************************************/
string ChatServer::RemoveMsgDestination(string msg)
{
    int start = msg.find(MSG_SRC_END)+2;
    int finish = msg.find(MSG_DST_END);
    return msg.erase(start, finish-start);
}

/*******************************************************************
 *
 *      ChatServer::GetMsgFilename
 *
 *******************************************************************/
string ChatServer::GetMsgFilename(string msg, char tag)
{
    int start, finish;
    switch(tag)
    {
        case SEND_FILE:
        case BLKCST_FILE:
            start = msg.find(MSG_DST_END)+1;
            break;
        case BRDCST_FILE:
            start = msg.find(MSG_SRC_END)+1;
            break;
        default:
            return "";
    }
    finish = msg.find(FILENAME_END);
    return msg.substr(start, finish-start);
}

/*******************************************************************
 *
 *      ChatServer::GetMsgData
 *
 *******************************************************************/
string ChatServer::GetMsgData(string msg, char tag)
{
    int start, finish;
    switch(tag)
    {
        case SEND_MSG:
        case BLKCST_MSG:
            start = msg.find(MSG_DST_END)+1;
            break;
        case BRDCST_MSG:
            start = msg.find(MSG_SRC_END)+1;
            break;
        default:
            return "";
    }
    finish = msg.find(MSG_END);
    return msg.substr(start, finish-start);
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

