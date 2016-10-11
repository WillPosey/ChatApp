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
    serverIP = serverPortStr = "";

    int octetStart = 0, octetEnd = 0, numOctets = 0;
    bool validIP = true, endFound = false;
    string octet;
    cout << "Server IPV4 Address: ";
    getline(cin, serverIP);
    while(validIP)
    {
        numOctets++;
        octet = "";
        if(octetStart >= serverIP.length())
        {
            validIP = false;
            continue;
        }
        octetEnd = serverIP.find('.', octetStart);
        endFound = (octetEnd == string::npos) ? true : false;
        octet = serverIP.substr(octetStart, octetEnd-octetStart);
        if(octet.length() > 3 || atoi(octet.c_str()) > 255)
        {
            validIP = false;
            continue;
        }
        for(int i=0; i<octet.length(); i++)
            if(!isdigit(octet[i]))
                validIP = false;
        octetStart = octetEnd+1;
        if(endFound)
        {
            if(numOctets==4)
                break;
            else
                validIP = false;
        }
        else if(numOctets==4)
            validIP = false;
    }

    if(!validIP)
    {
        cout << "Invalid IP Address" << endl;
        return SERVER_INFO;
    }

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
    cout << "Connecting to [" <<  serverIP << "] on Port [" << serverPortStr << "]..." << endl;

    struct addrinfo hints, *addr, *results = NULL;

    //define connection
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(serverIP.c_str(), serverPortStr.c_str(), &hints, &results) != 0)
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

    cout << "Connected to [" <<  serverIP << "] on Port [" << serverPortStr << "]" << endl;

    return USERNAME;
}

/*******************************************************************
 *
 *      ChatClient::GetUsername
 *
 *******************************************************************/
int ChatClient::GetUsername()
{
    return GET_USERS;
}

/*******************************************************************
 *
 *      ChatClient::GetOtherUsers
 *
 *******************************************************************/
int ChatClient::GetOtherUsers()
{
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
    RecvThread.join();
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
    string recvMsg, output;
    int i = 0;
    char c = 65;

    while((++i)!=6)
    {
        recvMsg = "";
        c++;
        recvMsg = c;
        output = "";
        int currentState = GET_TYPE;

        while(currentState != PARSE_COMPLETE)
            currentState = (this->*recvStateFunction[currentState])(recvMsg, output);
    }
    SetExit(true);
}
