/*******************************************************************
 *      ChatClient.cpp
 *
 *      Author: William Posey
 *      Course: CNT5106C
 *
 *******************************************************************/
#include "MessageDefs.h"
#include "ChatClient.h"
#include <cstdlib>
#include <strings.h>
#include <cstring>

using namespace std;

/*******************************************************************
 *
 *      Main Method
 *
 *******************************************************************/
int main()
{
    ChatClient client;
    client.StartClient();
    return 0;
}

/*******************************************************************
 *
 *      ChatClient Constructor
 *
 *******************************************************************/
ChatClient::ChatClient()
{
    exiting = serverExit = false;
    pthread_mutex_init(&exitLock, NULL);
    pthread_mutex_init(&shutdownLock, NULL);
    pthread_mutex_init(&displayLock, NULL);
}

/*******************************************************************
 *
 *      ChatClient Destructor
 *
 *******************************************************************/
ChatClient::~ChatClient()
{
    pthread_mutex_destroy(&exitLock);
    pthread_mutex_destroy(&shutdownLock);
    pthread_mutex_destroy(&displayLock);
}

/*******************************************************************
 *
 *      ChatClient::SetExit
 *
 *******************************************************************/
void ChatClient::SetExit(bool val)
{
    pthread_mutex_lock(&exitLock);
    exiting = val;
    pthread_mutex_unlock(&exitLock);
}

/*******************************************************************
 *
 *      ChatClient::CheckExit
 *
 *******************************************************************/
bool ChatClient::CheckExit()
{
    pthread_mutex_lock(&exitLock);
    bool retVal = exiting;
    pthread_mutex_unlock(&exitLock);
    return retVal;
}

/*******************************************************************
 *
 *      ChatClient::SetShutdown
 *
 *******************************************************************/
void ChatClient::SetShutdown(bool val)
{
    pthread_mutex_lock(&exitLock);
    serverExit = val;
    pthread_mutex_unlock(&exitLock);
}

/*******************************************************************
 *
 *      ChatClient::CheckShutdown
 *
 *******************************************************************/
bool ChatClient::CheckShutdown()
{
    pthread_mutex_lock(&exitLock);
    bool retVal = serverExit;
    pthread_mutex_unlock(&exitLock);
    return retVal;
}

/*******************************************************************
 *
 *      ChatClient::StartClient
 *
 *******************************************************************/
void ChatClient::StartClient()
{
    int currentState = SERVER_INFO;

    while(currentState != COMPLETE)
        currentState = (this->*setupStateFunction[currentState])();
}

/*******************************************************************
 *
 *      ChatClient::GetServerInfo
 *
 *******************************************************************/
int ChatClient::GetServerInfo()
{
    serverPortStr = "";
    bool validPort = true;

    cout << "Server Port: ";
    getline(cin, serverPortStr);
    serverPort = atoi(serverPortStr.c_str());
    if( serverPort < 1024 || serverPort > 65535)
        validPort = false;
    for(int i=0; i<serverPortStr.length(); i++)
        if(!isdigit(serverPortStr[i]))
            validPort = false;

    if(!validPort)
    {
        cout << "Invalid Port" << endl;
        return SERVER_INFO;
    }

    return CONNECT;
}

/*******************************************************************
 *
 *      ChatClient::ConnectToServer
 *
 *******************************************************************/
int ChatClient::ConnectToServer()
{
    cout << "Connecting to Chat Server on Port [" << serverPortStr << "]..." << endl;

    struct addrinfo hints, *addr, *results = NULL;

    //define connection
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, serverPortStr.c_str(), &hints, &results) != 0)
    {
        cout << "Error: could not resolve host information" << endl;
        return SERVER_INFO;
    }

    socket_fd = -1; //socket descriptor

    //create the socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Error: socket creation failed" << endl;
        return SERVER_INFO;
    }

    //attempt to connect the socket for all addresses
    for(addr = results; addr != NULL; addr = addr->ai_next )
    {
        socket_fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if(socket_fd < 0)
            continue;

        //connect to the server
        if(connect(socket_fd, addr->ai_addr, addr->ai_addrlen) == 0)
            break;  //connection succeeded

        close(socket_fd);  //connection unnsuccesful
        socket_fd = -1;
    }

    freeaddrinfo(results);

    if(socket_fd == -1)
    {
        cout << "Error: socket creation failed" << endl;
        return SERVER_INFO;
    }

    cout << "Connected to Chat Server on Port [" << serverPortStr << "]" << endl;

    return USERNAME;
}

/*******************************************************************
 *
 *      ChatClient::GetUsername
 *
 *******************************************************************/
int ChatClient::GetUsername()
{
    bool validUsername, accepted = false;
    char serverMsg, submit = USERNAME_SUBMIT;
    string usernameMsg;

    while(1)
    {
        recv(socket_fd, &serverMsg, 1, 0);
        if(serverMsg == USERNAME_REQUEST)
            break;
    }

    while(!accepted)
    {
        validUsername = false;
        while(!validUsername)
        {
            cout << "[Server] Username: ";
            username = "";
            getline(cin, username);
            if(username.find(' ') != string::npos)
                cout << "[Server] Usernames may not contain spaces" << endl;
            else
                validUsername = true;
        }
        usernameMsg = "";
        usernameMsg += submit;
        usernameMsg += username;
        usernameMsg += MSG_END;
        send(socket_fd, usernameMsg.c_str(), usernameMsg.length(), 0);
        recv(socket_fd, &serverMsg, 1, 0);
        switch(serverMsg)
        {
            case USERNAME_REQUEST:
                cout << "[Server] " << username << " is not available" << endl;
                break;
            case USERNAME_VALID:
                cout << "[" << username << "] now connected" << endl;
                accepted = true;
                break;
            default:
                cout << "ERROR: could not send username to Server" << endl;
                return SERVER_INFO;
        }
    }

    return GET_USERS;
}

/*******************************************************************
 *
 *      ChatClient::GetOtherUsers
 *
 *******************************************************************/
int ChatClient::GetOtherUsers()
{
    char users[100];
    int numBytes, index, start = 0;
    char getUsers = OTHER_USERS_REQUEST;
    send(socket_fd, &getUsers, 1, 0);

    string usersStr = "";

    /* Retrieve Client's Username */
    do
    {
        memset(users, 0, 100);
        numBytes = recv(socket_fd, users, 100, 0);
        if(numBytes < 0)
        {
            cout << "ERROR: could not receive other users" << endl;
            return SERVER_INFO;
        }
        usersStr += string(users);
    } while(users[numBytes-1] != MSG_END);

    usersStr.erase(0,1);    //remove USERNAMES_REPLY tag
    usersStr.pop_back();    //remove END_MSG tag
    do
    {
        index = usersStr.find(USERNAME_END, start);
        otherUsers.push_back(usersStr.substr(start, index));
        start = index+1;
    }while(index != string::npos);

    cout << "Other Users: ";
    for(vector<string>::iterator currentUser = otherUsers.begin(); currentUser != otherUsers.end(); currentUser++)
        cout << *currentUser << " ";
    cout << endl;

    return START_THREADS;
}

/*******************************************************************
 *
 *      ChatClient::SetupComplete
 *
 *******************************************************************/
int ChatClient::SetupComplete()
{
    SendThread = thread(&ChatClient::ClientSend, this);
    RecvThread = thread(&ChatClient::ClientRecv, this);
    SendThread.join();
    //RecvThread.join();
    return COMPLETE;
}

/*******************************************************************
 *
 *      ChatClient::ClientSend
 *
 *******************************************************************/
void ChatClient::ClientSend()
{
    string input, output;

    int currentState = GET_INPUT;

    while(!CheckExit())
        currentState = (this->*sendStateFunction[currentState])(input, output);
}

/*******************************************************************
 *
 *      ChatClient::ClientRecv
 *
 *******************************************************************/
void ChatClient::ClientRecv()
{
    string msg, recvMsg, output;
    int currentState;

    while(1)
    {
        currentState = GET_TYPE;

        msg = ReceiveFromServer();
        cout << msg.substr(1,msg.length() - 1) << "disconnected" << endl;

        while(currentState != PARSE_COMPLETE)
            currentState = (this->*recvStateFunction[currentState])(recvMsg, output);
    }
    SetExit(true);
}

/*******************************************************************
 *
 *      ChatClient::ReceiveFromServer
 *
 *******************************************************************/
string ChatClient::ReceiveFromServer()
{
    string currentMsg;
    char msg[BUFFER_LENGTH];
    int numBytes;

    do
    {
        memset(msg, 0, BUFFER_LENGTH);
        numBytes = recv(socket_fd, msg, BUFFER_LENGTH, 0);
        if(numBytes < 0)
        {
            cout << "ERROR: could not receive from server" << endl;
            return "";
        }
        currentMsg += string(msg);
    } while(msg[numBytes-1] != MSG_END);

    return currentMsg;
}
