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
    pthread_mutex_init(&disconnectLock, NULL);
    pthread_mutex_init(&displayLock, NULL);
    pthread_mutex_init(&promptCheckLock, NULL);
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
    pthread_mutex_destroy(&disconnectLock);
    pthread_mutex_destroy(&displayLock);
    pthread_mutex_destroy(&promptCheckLock);
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
 *      ChatClient::SetDisconnect
 *
 *******************************************************************/
void ChatClient::SetDisconnect(bool val)
{
    pthread_mutex_lock(&disconnectLock);
    disconnect = val;
    pthread_mutex_unlock(&disconnectLock);
}

/*******************************************************************
 *
 *      ChatClient::CheckDisconnect
 *
 *******************************************************************/
bool ChatClient::CheckDisconnect()
{
    pthread_mutex_lock(&disconnectLock);
    bool retVal = disconnect;
    pthread_mutex_unlock(&disconnectLock);
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
 *      ChatClient::SetPromptDisplayed
 *
 *******************************************************************/
void ChatClient::SetPromptDisplayed(bool val)
{
    pthread_mutex_lock(&promptCheckLock);
    promptDisplayed = val;
    pthread_mutex_unlock(&promptCheckLock);
}

/*******************************************************************
 *
 *      ChatClient::CheckPromptDisplayed
 *
 *******************************************************************/
bool ChatClient::CheckPromptDisplayed()
{
    pthread_mutex_lock(&promptCheckLock);
    bool retVal = promptDisplayed;
    pthread_mutex_unlock(&promptCheckLock);
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

    while(currentState != CLIENT_EXIT)
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
    if(serverPortStr.compare(EXIT_CMD) == 0)
        return CLIENT_EXIT;
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
    return COMPLETE;
}

/*******************************************************************
 *
 *      ChatClient::WaitForExit
 *
 *******************************************************************/
int ChatClient::WaitForExit()
{
    SendThread.join();
    RecvThread.join();
    if(CheckShutdown())
    {
        cout << "Server shutdown; please reconnect" << endl;
        return SERVER_INFO;
    }
    else if(CheckDisconnect())
    {
        cout << "Client Disconnecting" << endl;
        return SERVER_INFO;
    }
    else if(CheckExit())
    {
        cout << "Client Exiting" << endl;
        return CLIENT_EXIT;
    }
    else
    {
        cout << "Error occured; exiting" << endl;
        return CLIENT_EXIT;
    }
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

    while(currentState != END_THREAD)
        currentState = (this->*sendStateFunction[currentState])(input, output);
}

/*******************************************************************
 *
 *      ChatClient::GetInput
 *
 *******************************************************************/
int ChatClient::GetInput(string& input, string& output)
{
    DisplayPrompt();
    getline(cin, input);
    SetPromptDisplayed(false);
    if(CheckShutdown())
        return END_THREAD;
    return PARSE_INPUT;
}

/*******************************************************************
 *
 *      ChatClient::ParseInput
 *
 *******************************************************************/
int ChatClient::ParseInput(string& input, string& output)
{
    int firstSpace;
    string cmd;

    firstSpace = input.find_first_of(' ');
    //maybe change to let if statement carry on in the event of not being exit or disconnect
    if(firstSpace == string::npos)
    {
        if(input.compare(EXIT_CMD))
        {
            SetExit(true);
            DisplayMsg("Shutting Down...");
            output = CLIENT_SHUTDOWN;
            return END_THREAD;
        }
        else if(input.compare(DISCONNECT_CMD))
        {
            SetDisconnect(true);
            DisplayMsg("Disconnecting From Server...");
            output = CLIENT_SHUTDOWN;
            return END_THREAD;
        }
        else
            DisplayMsg("Error: Incorrect Input");
            return GET_INPUT;
    }

    cmd = input.substr(0, firstSpace);
    if(cmd.compare(SEND_CMD))
    {

    }
    else if(cmd.compare(SEND_FILE_CMD))
    {

    }
    else if(cmd.compare(BRDCST_CMD))
    {

    }
    else if(cmd.compare(BRDCST_FILE_CMD))
    {

    }
    else if(cmd.compare(BLKCST_CMD))
    {

    }
    else if(cmd.compare(BLKCST_FILE_CMD))
    {

    }

    return SEND_INPUT;
}

/*******************************************************************
 *
 *      ChatClient::SendMsg
 *
 *******************************************************************/
int ChatClient::SendMsg(string& input, string& output)
{
    if(send(socket_fd, output.c_str(), output.length(), 0) < 0)
        DisplayMsg("ERROR: could not send message to server");
    return GET_INPUT;
}

/*******************************************************************
 *
 *      ChatClient::ClientRecv
 *
 *******************************************************************/
void ChatClient::ClientRecv()
{
    string recvMsg, output;
    int currentState;

    while(1)
    {
        if(CheckDisconnect() || CheckExit() || CheckShutdown())
            break;

        currentState = GET_TYPE;

        recvMsg = ReceiveFromServer();

        while(currentState != PARSE_COMPLETE)
            currentState = (this->*recvStateFunction[currentState])(recvMsg, output);
    }
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

/*******************************************************************
 *
 *      ChatClient::GetMsgType
 *
 *******************************************************************/
int ChatClient::GetMsgType(string msg, string& display)
{
    //recvMsgType = ;
    return GET_SRC;
}

/*******************************************************************
 *
 *      ChatClient::GetSource
 *
 *******************************************************************/
int ChatClient::GetSource(string msg, string& display)
{
    //if source = server
    //{
    //  if(recvMsgType == USER_DISCONNECT)....
    //      return ...
    //  if(recvMsgType == USER_CONNECT)....
    //      return ...
    //  if(recvMsgType == SERVER_SHUTDOWN)....
    //      return ...
    //}

    //if(recvMsgType == MSG_RCV)....
    return GET_MSG;
    //if(recvMsgType == FILE_RCV)....
    return GET_FILENAME;
}

/*******************************************************************
 *
 *      ChatClient::GetFilename
 *
 *******************************************************************/
int ChatClient::GetFilename(string msg, string& display)
{
    return GET_FILE;
}

/*******************************************************************
 *
 *      ChatClient::GetMsg
 *
 *******************************************************************/
int ChatClient::GetMsg(string msg, string& display)
{
    return DISPLAY;
}

/*******************************************************************
 *
 *      ChatClient::GetFile
 *
 *******************************************************************/
int ChatClient::GetFile(string msg, string& display)
{
    return DISPLAY;
}

/*******************************************************************
 *
 *      ChatClient::DisplayRecvMsg
 *
 *******************************************************************/
int ChatClient::DisplayRecvMsg(string msg, string& display)
{
    DisplayMsg(display);
    return PARSE_COMPLETE;
}



/*******************************************************************
 *
 *      ChatClient::DisplayMsg
 *
 *******************************************************************/
void ChatServer::DisplayMsg(string msg)
{
    pthread_mutex_lock(&displayLock);
    bool insertLineAbove = CheckPromptDisplayed();
    if(insertLineAbove)
        //insert line above the displayed prompt used in getline?
    else
        cout << msg << endl;
    pthread_mutex_unlock(&displayLock);
}

/*******************************************************************
 *
 *      ChatClient::DisplayPrompt
 *
 *******************************************************************/
void ChatServer::DisplayPrompt(string msg)
{
    SetPromptDisplayed(true);
    DisplayMsg("[" + username + "]: ");
}
