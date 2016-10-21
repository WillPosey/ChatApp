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
    exiting = serverExit = disconnect = promptDisplayed = false;
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

    printf("\033[1A\033[KConnected to Chat Server on Port [%s]\n", serverPortStr.c_str() );

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
            cout << "Username: ";
            username = "";
            getline(cin, username);
            if(username.find(' ') != string::npos)
                cout << "Usernames may not contain spaces" << endl;
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
                cout << username << " is not available" << endl;
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
    int retVal;

    SendThread.join();
    RecvThread.join();

    if(CheckShutdown())
    {
        cout << "Server shutdown; please reconnect" << endl;
        retVal = SERVER_INFO;
    }
    else if(CheckDisconnect())
    {
        cout << "Client Disconnecting" << endl;
        retVal = SERVER_INFO;
    }
    else if(CheckExit())
    {
        cout << "Client Exiting" << endl;
        retVal = CLIENT_EXIT;
    }
    else
    {
        cout << "Error occured; exiting" << endl;
        retVal = CLIENT_EXIT;
    }

    SetDisconnect(false);
    SetExit(false);
    SetShutdown(false);
    close(socket_fd);
    return retVal;
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
    output = "";
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
    int firstSpace, secondSpace;
    string cmd, destination, msg, filename;
    char tag;

    firstSpace = input.find_first_of(' ');
    //maybe change to let if statement carry on in the event of not being exit or disconnect
    if(firstSpace == string::npos)
    {
        // exit
        if(input.compare(EXIT_CMD) == 0)
        {
            tag = CLIENT_SHUTDOWN;
            SetExit(true);
            DisplayMsg("Shutting Down...");
            output += tag;
        }
        // disconnect
        else if(input.compare(DISCONNECT_CMD) == 0)
        {
            tag = CLIENT_SHUTDOWN;
            SetDisconnect(true);
            DisplayMsg("Disconnecting From Server...");
            output += tag;
        }
        // error
        else
        {
            DisplayMsg("Error: Incorrect Input");
            return GET_INPUT;
        }
    }
    else
    {
        cmd = input.substr(0, firstSpace);
        secondSpace = input.find_first_of(' ', firstSpace+1);

        // send msg
        if(cmd.compare(SEND_CMD) == 0)
        {
            tag = SEND_MSG;
            destination = input.substr(firstSpace+1, secondSpace-firstSpace-1);
            msg = input.substr(secondSpace+1, input.length()-secondSpace-1);
            output += tag + username + MSG_SRC_END + destination + MSG_DST_END + msg;
        }
        // send file
        else if(cmd.compare(SEND_FILE_CMD) == 0)
        {
            tag = SEND_FILE;
            destination = input.substr(firstSpace+1, secondSpace-firstSpace-1);
            filename = input.substr(secondSpace+1, input.length()-secondSpace-1);
            output += tag + username + MSG_SRC_END + destination + MSG_DST_END + filename + FILENAME_END;
            //get file
        }
        // broadcast msg
        else if(cmd.compare(BRDCST_CMD) == 0)
        {
            tag = BRDCST_MSG;
            msg = input.substr(firstSpace+1, input.length()-firstSpace-1);
            output += tag + msg;
        }
        // broadcast file
        else if(cmd.compare(BRDCST_FILE_CMD) == 0)
        {
            tag = BRDCST_FILE;
            filename = input.substr(firstSpace+1, input.length()-firstSpace-1);
            output += tag + username + MSG_SRC_END + filename + FILENAME_END;
            //get file
        }
        // blockcast msg
        else if(cmd.compare(BLKCST_CMD) == 0)
        {
            tag = BLKCST_MSG;
            destination = input.substr(firstSpace+1, secondSpace-firstSpace-1);
            msg = input.substr(secondSpace+1, input.length()-secondSpace-1);
            output += tag + username + MSG_SRC_END + destination + MSG_DST_END + msg;
        }
        // blockcast file
        else if(cmd.compare(BLKCST_FILE_CMD) == 0)
        {
            tag = BLKCST_FILE;
            destination = input.substr(firstSpace+1, secondSpace-firstSpace-1);
            filename = input.substr(secondSpace+1, input.length()-secondSpace-1);
            output += tag + username + MSG_SRC_END + destination + MSG_DST_END + filename + FILENAME_END;
            //get file
        }
    }
    output += MSG_END;
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
    if(CheckDisconnect() || CheckExit() || CheckShutdown())
        return END_THREAD;
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
    bool loopRecv = true;

    while(loopRecv)
    {
        if(CheckDisconnect() || CheckExit() || CheckShutdown())
            break;

        currentState = GET_TYPE;

        while(!ReceiveFromServer(recvMsg))
        {
            if(CheckDisconnect() || CheckExit() || CheckShutdown())
            {
                loopRecv = false;
                break;
            }
        }
        if(!loopRecv)
            continue;

        output = "";

        while(currentState != PARSE_COMPLETE)
            currentState = (this->*recvStateFunction[currentState])(recvMsg, output);
    }
}

/*******************************************************************
 *
 *      ChatClient::ReceiveFromServer
 *
 *******************************************************************/
bool ChatClient::ReceiveFromServer(string& recvMsg)
{
    string currentMsg;
    char msg[BUFFER_LENGTH];
    int numBytes;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    do
    {
        memset(msg, 0, BUFFER_LENGTH);
        numBytes = recv(socket_fd, msg, BUFFER_LENGTH, 0);
        if(numBytes < 0)
        {
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
            {
                cout << "ERROR: could not receive from server" << endl;
                return false;
            }
            //timeout
            else
            {
                return false;
            }
        }
        currentMsg += string(msg);
    } while(msg[numBytes-1] != MSG_END);

    recvMsg = currentMsg;
    return true;
}

/*******************************************************************
 *
 *      ChatClient::GetMsgType
 *
 *******************************************************************/
int ChatClient::GetMsgType(string msg, string& display)
{
    recvMsgType = msg[0];
    return GET_SRC;
}

/*******************************************************************
 *
 *      ChatClient::GetSource
 *
 *******************************************************************/
int ChatClient::GetSource(string msg, string& display)
{
    if(recvMsgType == USER_DISCONNECT)
    {
        display += "[" + msg.substr(1,msg.length()-2) + "]->DISCONNECTED";
        return DISPLAY;
    }
    if(recvMsgType == USER_CONNECT)
    {
        display += "[" + msg.substr(1,msg.length()-2) + "]->CONNECTED";
        return DISPLAY;
    }
    if(recvMsgType == SERVER_SHUTDOWN)
    {
        display += "Server Shutting Down...";
        SetShutdown(true);
        return DISPLAY;
    }

    srcEnd = msg.find(MSG_SRC_END);
    display += "[" + msg.substr(1,srcEnd-1) + "]->";

    if(recvMsgType == MSG_RCV)
        return GET_MSG;
    else if(recvMsgType == FILE_RCV)
        return GET_FILENAME;
    else
        return PARSE_COMPLETE;
}

/*******************************************************************
 *
 *      ChatClient::GetFilename
 *
 *******************************************************************/
int ChatClient::GetFilename(string msg, string& display)
{
    filenameEnd = msg.find(FILENAME_END);
    filename = msg.substr(srcEnd+1, filenameEnd-srcEnd+1);
    display += "File: " + filename;
    return GET_FILE;
}

/*******************************************************************
 *
 *      ChatClient::GetMsg
 *
 *******************************************************************/
int ChatClient::GetMsg(string msg, string& display)
{
    display += msg.substr(srcEnd+1, msg.length()-srcEnd);
    return DISPLAY;
}

/*******************************************************************
 *
 *      ChatClient::GetFile
 *
 *******************************************************************/
int ChatClient::GetFile(string msg, string& display)
{
    //actually get the entire file and save it to current directory
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
void ChatClient::DisplayMsg(string msg, bool newline)
{
    pthread_mutex_lock(&displayLock);
    bool insertLineAbove = CheckPromptDisplayed();
    //if(insertLineAbove)
        //insert line above the displayed prompt used in getline?
    //else
        cout << msg << endl;
    if(newline)
        cout << endl;
    pthread_mutex_unlock(&displayLock);
}

/*******************************************************************
 *
 *      ChatClient::DisplayPrompt
 *
 *******************************************************************/
void ChatClient::DisplayPrompt()
{
    SetPromptDisplayed(true);
    DisplayMsg("[" + username + "]: ", false);
}
