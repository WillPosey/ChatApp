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
#include <sys/stat.h>
#include <cstring>
#include <algorithm>

using namespace std;

/*******************************************************************
 *
 *      Main Method
 *
 *******************************************************************/
int main(int argc, char** argv)
{
    if(argc == 3)
    {
        ChatClient client (argv[1], argv[2]);
        client.StartClient();
    }
    else
        cout << "Please specify username and server port" << endl;
    return 0;
}

/*******************************************************************
 *
 *      ChatClient Constructor
 *
 *******************************************************************/
ChatClient::ChatClient(char* clientUsername, char* port)
{
    username = string(clientUsername);
    prompt = "[" + username + "]: ";
    serverPortStr = string(port);
    serverPort = atoi(port);
    clientExiting = serverShutdown = promptDisplayed = false;
    pthread_mutex_init(&displayLock, NULL);
    pthread_mutex_init(&promptCheckLock, NULL);
    signalDetected = 0;
    signal(SIGINT, signalHandler);
}

/*******************************************************************
 *
 *      ChatClient Destructor
 *
 *******************************************************************/
ChatClient::~ChatClient()
{
    pthread_mutex_destroy(&displayLock);
    pthread_mutex_destroy(&promptCheckLock);
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
 *      ChatClient::GetTag
 *
 *******************************************************************/
string ChatClient::GetTag(string msg)
{
    return msg.substr(0,2);
}

/*******************************************************************
 *
 *      ChatClient::GetCmd
 *
 *******************************************************************/
string ChatClient::GetCmd(string input)
{
    int finish = input.find(MSG_TAG);
    if(finish == string::npos)
        return input;
    return input.substr(0, finish);
}

/*******************************************************************
 *
 *      ChatClient::CompareTag
 *
 *******************************************************************/
bool ChatClient::CompareTag(string tag, string checkTag)
{
    return (tag.compare(checkTag) == 0);
}

/*******************************************************************
 *
 *      ChatClient::CompareCmd
 *
 *******************************************************************/
bool ChatClient::CompareCmd(string cmd, string checkCmd)
{
    return (cmd.compare(checkCmd) == 0);
}

/*******************************************************************
 *
 *      ChatClient::UserExists
 *
 *******************************************************************/
bool ChatClient::UserExists(string user)
{
    vector<string>::iterator userIterator;

    for(userIterator = otherUsers.begin(); userIterator != otherUsers.end(); userIterator++)
        if(user.compare(*userIterator) == 0)
            return true;
    return false;
}

/*******************************************************************
 *
 *      ChatClient::FileExists
 *
 *******************************************************************/
bool ChatClient::FileExists(string filename)
{
    struct stat buffer;
    return (stat (filename.c_str(), &buffer) == 0);
}

/*******************************************************************
 *
 *      ChatClient::GetDestination
 *
 *******************************************************************/
string ChatClient::GetDestination(string msg)
{
    int start = msg.find(MSG_TAG)+1;
    start = msg.find(MSG_TAG, start)+2;
    int finish = msg.length();
    return msg.substr(start, finish-start);
}

/*******************************************************************
 *
 *      ChatClient::GetSource
 *
 *******************************************************************/
string ChatClient::GetSource(string msg)
{
    int start = msg.find(USER_TAG)+1;
    int finish = msg.find(MSG_TAG);
    if(finish == string::npos)
        finish = msg.length()-1;
    return msg.substr(start, finish-start);
}

/*******************************************************************
 *
 *      ChatClient::GetFilename
 *
 *******************************************************************/
string ChatClient::GetFilename(string msg)
{
    int start = msg.find(MSG_TAG)+1;
    int finish = msg.find(MSG_TAG, start);
    return msg.substr(start, finish-start);
}

/*******************************************************************
 *
 *      ChatClient::GetMsg
 *
 *******************************************************************/
string ChatClient::GetMsg(string msg)
{
    int start = msg.find(MSG_TAG)+1;
    int finish = msg.find(MSG_TAG, start);
    return msg.substr(start, finish-start);
}

/*******************************************************************
 *
 *      ChatClient::GetFile
 *
 *******************************************************************/
string ChatClient::GetFile(string msg)
{
    int start = msg.find(MSG_TAG)+1;
    start = msg.find(MSG_TAG, start)+1;
    return msg.substr(start, msg.length()-start-1);
}

/*******************************************************************
 *
 *      ChatClient::CreateFile
 *
 *******************************************************************/
void ChatClient::CreateFile(string filename, string file)
{
    ofstream outStream(basename(filename.c_str()));
    outStream << file;
    outStream.close();
}

/*******************************************************************
 *
 *      ChatClient::ReadFile
 *
 *******************************************************************/
string ChatClient::ReadFile(string filename)
{
    ifstream inStream (filename.c_str(), ios::in | ios::binary | ios::ate);
    ifstream::pos_type fileSize = inStream.tellg();
    inStream.seekg(0, ios::beg);

    vector<char> bytes(fileSize);
    inStream.read(&bytes[0], fileSize);

    return string(&bytes[0], fileSize);
}

/*******************************************************************
 *
 *      ChatClient::StartClient
 *
 *******************************************************************/
void ChatClient::StartClient()
{
    if(!ConnectToServer())
        return;
    SendThread = thread(&ChatClient::ClientSend, this);
    RecvThread = thread(&ChatClient::ClientRecv, this);
    send_t = SendThread.native_handle();
    recv_t = RecvThread.native_handle();
    WaitForExit();
}

/*******************************************************************
 *
 *      ChatClient::ConnectToServer
 *
 *******************************************************************/
bool ChatClient::ConnectToServer()
{
    struct addrinfo hints, *addr, *results = NULL;
    string recvMsg, recvTag, usernameMsg, neighborName;
    int startIndex;

    //define connection
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if(getaddrinfo(NULL, serverPortStr.c_str(), &hints, &results) != 0)
    {
        cout << "Error: could not resolve host information" << endl;
        return false;
    }

    socket_fd = -1; //socket descriptor

    //create the socket
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Error: socket creation failed" << endl;
        return false;
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
        DisplayMsg("Error: socket creation failed");
        return false;
    }

    // Wait for username request from server, then send username
    // if username is taken, USERNAME_TAKEN message is returned
    // if username available, list of other users is sent in OTHER_USERS message
    recvMsg = ReceiveFromServer();
    recvTag = GetTag(recvMsg);
    usernameMsg = username + MSG_END;
    if(CompareTag(recvTag, USERNAME_REQUEST))
    {
        SendMsg(usernameMsg);
        recvMsg = ReceiveFromServer();
        recvTag = GetTag(recvMsg);
        if(CompareTag(recvTag, OTHER_USERS))
        {
            DisplayMsg("Connected to Server on Port " + serverPortStr);
            DisplayMsg("Other Users: ", false);
            while(GetNextNeighbor(recvMsg, neighborName))
            {
                DisplayMsg(neighborName + " ", false);
                AddNeighbor(neighborName);
            }
            DisplayMsg("");
            return true;
        }
        else
            DisplayMsg("Error: Username is taken");
    }
    else
        DisplayMsg("Error: Could not establish username with server");

    return false;
}

/*******************************************************************
 *
 *      ChatClient::GetNextNeighbor
 *
 *******************************************************************/
bool ChatClient::GetNextNeighbor(string neighbors, string& name)
{
    static int start = 0;
    int finish;

    start = neighbors.find(USER_TAG, start);
    if(start == string::npos)
        return false;
    start++;
    finish = neighbors.find(USER_TAG, start);
    if(finish == string::npos)
        finish = neighbors.length()-1;
    name = neighbors.substr(start, finish-start);
    return true;
}

/*******************************************************************
 *
 *      ChatClient::RemoveNeighbor
 *
 *******************************************************************/
void ChatClient::RemoveNeighbor(string name)
{
    vector<string>::iterator usernameIt;

    usernameIt = find(otherUsers.begin(), otherUsers.end(), name);
    otherUsers.erase(usernameIt);
}

/*******************************************************************
 *
 *      ChatClient::WaitForExit
 *
 *******************************************************************/
void ChatClient::WaitForExit()
{
    SendThread.join();
    RecvThread.join();

    if(serverShutdown)
        DisplayMsg("Server Shutdown");
    else if(clientExiting)
        DisplayMsg("Client Exiting");
    else
        DisplayMsg("Error Occured, Client Exiting");
}

/*******************************************************************
 *
 *      ChatClient::ClientSend
 *
 *******************************************************************/
void ChatClient::ClientSend()
{
    string input, output;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

    while(1)
    {
        pthread_testcancel();
        GetInput(input);
        if(ParseInput(input, output))
            SendMsg(output);
        if(clientExiting)
        {
            TerminateThreads();
            break;
        }
    }
}

/*******************************************************************
 *
 *      ChatClient::GetInput
 *
 *******************************************************************/
void ChatClient::GetInput(string& input)
{
    fd_set fdSet;
    struct timeval timeout;
    char inputChar = 0;

    input = "";
    currentInput = "";
    DisplayPrompt();

    while(1)
    {
        FD_ZERO (&fdSet);
        FD_SET (STDIN_FILENO, &fdSet);

        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        select(STDIN_FILENO+1, &fdSet, NULL, NULL, &timeout);
        pthread_testcancel();
        if(signalDetected)
        {
            input = "exit";
            break;
        }
        if(FD_ISSET(STDIN_FILENO, &fdSet))
        {
            read(STDIN_FILENO, &inputChar, 1);
            if(inputChar == '\n')
            {
                SetPromptDisplayed(false);
                break;
            }
            input += inputChar;
            pthread_mutex_lock(&promptCheckLock);
            currentInput += inputChar;
            pthread_mutex_unlock(&promptCheckLock);
        }
    }
}

/*******************************************************************
 *
 *      ChatClient::ParseInput
 *
 *******************************************************************/
bool ChatClient::ParseInput(string& input, string& output)
{
    string cmd, msg, filename, destination, file;

    cmd = GetCmd(input);

    // exit
    if(CompareCmd(cmd, EXIT_CMD))
    {
        clientExiting = true;
        output = string(CLIENT_SHUTDOWN) + MSG_END;
        return true;
    }

    // broadcast message
    if(CompareCmd(cmd, BRDCST_CMD))
    {
        msg = GetMsg(input);
        output = string(BRDCST_MSG) + MSG_TAG + msg + MSG_TAG + MSG_END;
        return true;
    }

    // broadcast file
    if(CompareCmd(cmd, BRDCST_FILE_CMD))
    {
        filename = GetFilename(input);
        if(!FileExists(filename))
        {
            DisplayMsg("\nError: file [" + filename + "] does not exist");
            return false;
        }
        filename = basename(filename.c_str());
        file = ReadFile(filename);
        output = string(BRDCST_FILE) + MSG_TAG + filename + MSG_TAG + file + MSG_END;
        return true;
    }

    // send message
    if(CompareCmd(cmd, SEND_CMD))
    {
        // check if destination exists
        destination = GetDestination(input);
        if(!UserExists(destination))
        {
            DisplayMsg("\nError: user [" + destination + "] not found");
            return false;
        }
        msg = GetMsg(input);
        output = string(SEND_MSG) + USER_TAG + destination + MSG_TAG + msg + MSG_TAG + MSG_END;
        return true;
    }

    // send file
    if(CompareCmd(cmd, SEND_FILE_CMD))
    {
        // check if destination exists
        destination = GetDestination(input);
        if(!UserExists(destination))
        {
            DisplayMsg("\nError: user [" + destination + "] not found");
            return false;
        }
        filename = GetFilename(input);
        if(!FileExists(filename))
        {
            DisplayMsg("\nError: file [" + filename + "] does not exist");
            return false;
        }
        filename = basename(filename.c_str());
        file = ReadFile(filename);
        output = string(SEND_FILE) + USER_TAG + destination + MSG_TAG + filename + MSG_TAG + file + MSG_END;
        return true;
    }

    // blockcast message
    if(CompareCmd(cmd, BLKCST_CMD))
    {
        // check if destination exists
        destination = GetDestination(input);
        if(!UserExists(destination))
        {
            DisplayMsg("\nError: user [" + destination + "] not found");
            return false;
        }
        msg = GetMsg(input);
        output = string(BLKCST_MSG) + USER_TAG + destination + MSG_TAG + msg + MSG_TAG + MSG_END;
        return true;
    }

    // blockcast file
    if(CompareCmd(cmd, BLKCST_FILE_CMD))
    {
        // check if destination exists
        destination = GetDestination(input);
        if(!UserExists(destination))
        {
            DisplayMsg("\nError: user [" + destination + "] not found");
            return false;
        }
        filename = GetFilename(input);
        if(!FileExists(filename))
        {
            DisplayMsg("\nError: file [" + filename + "] does not exist");
            return false;
        }
        filename = basename(filename.c_str());
        file = ReadFile(filename);
        output = string(BLKCST_FILE) + USER_TAG + destination + MSG_TAG + filename + MSG_TAG + file + MSG_END;
        return true;
    }

    DisplayMsg("\nError: Command Not Found");
    return false;

}

/*******************************************************************
 *
 *      ChatClient::SendMsg
 *
 *******************************************************************/
void ChatClient::SendMsg(string msg)
{
    if(send(socket_fd, msg.c_str(), msg.length(), 0) < 0)
        DisplayMsg("ERROR: could not send message to server");
}

/*******************************************************************
 *
 *      ChatClient::ClientRecv
 *
 *******************************************************************/
void ChatClient::ClientRecv()
{
    string recvMsg, display;

    while(1)
    {
        pthread_testcancel();
        recvMsg = ReceiveFromServer();
        ParseMessage(recvMsg, display);
        if(serverShutdown)
        {
            TerminateThreads();
            break;
        }
        DisplayMsg(display);
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
            DisplayMsg("ERROR: could not receive from server");
            return "";
        }
        currentMsg += string(msg);
    } while(msg[numBytes-1] != MSG_END);

    return currentMsg;
}


/*******************************************************************
 *
 *      ChatClient::ParseMessage
 *
 *******************************************************************/
void ChatClient::ParseMessage(string input, string &display)
{
    string tag, source, filename, msg, file;

    tag = GetTag(input);

    // server shutdown
    if(CompareTag(tag, SERVER_SHUTDOWN))
    {
        serverShutdown = true;
        return;
    }

    source = GetSource(input);

    // new user connected
    if(CompareTag(tag, USER_CONNECT))
    {
        AddNeighbor(source);
        DisplayMsg(source + " connnected");
        return;
    }

    // user disconnected
    if(CompareTag(tag, USER_DISCONNECT))
    {
        RemoveNeighbor(source);
        DisplayMsg(source + " disconnnected");
        return;
    }

    // message received
    if(CompareTag(tag, MSG_RCV))
    {
        msg = GetMsg(input);
        DisplayMsg(source + ": " + msg);
        return;
    }

    // file received
    if(CompareTag(tag, FILE_RCV))
    {
        filename = GetFilename(input);
        file = GetFile(input);
        CreateFile(filename, file);
        DisplayMsg("File: [" + filename + "] sent by " + source);
        return;
    }
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
    if(insertLineAbove)
    {
        // not printing the prompt on line after current line?!
        // should go:
        // 1) prompt on line x
        // 2) display output on line x
        // 3) display prompt and input again on line x+1
        // but
        // 3) displays prompt on line x+2?
        pthread_mutex_lock(&promptCheckLock);
        printf("\r\033[K");
        fflush(stdout);
        printf("\r%s", msg.c_str());
        fflush(stdout);
        printf("\n");
        fflush(stdout);
        printf("%s", prompt.c_str());
        fflush(stdout);
        /*int length = prompt.length() + currentInput.length() + 1;
        while(--length >= 0)
        {
            printf("\033[1C");
            fflush(stdout);
        }*/
        pthread_mutex_unlock(&promptCheckLock);
    }
    else
    {
        cout << msg;
        if(newline)
            cout << endl;
    }
    /*cout << msg;
    if(newline)
        cout << endl;*/
    flush(cout);
    pthread_mutex_unlock(&displayLock);
}

/*******************************************************************
 *
 *      ChatClient::DisplayPrompt
 *
 *******************************************************************/
void ChatClient::DisplayPrompt()
{
    pthread_mutex_lock(&displayLock);
    cout << prompt;
    flush(cout);
    SetPromptDisplayed(true);
    pthread_mutex_unlock(&displayLock);
}
