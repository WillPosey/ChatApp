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
#include <cstring>
#include <algorithm>
#include <sstream>
#include <termios.h>
#include <strings.h>
#include <sys/stat.h>

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
    prompt = "[" + username + "] # ";
    serverPortStr = string(port);
    serverPort = atoi(port);
    clientExiting = serverShutdown = promptDisplayed = false;
    signalDetected = 0;
    signal(SIGINT, signalHandler);
}

/*******************************************************************
 *
 *      ChatClient::SetPromptDisplayed
 *
 *******************************************************************/
void ChatClient::SetPromptDisplayed(bool val)
{
    promptCheckLock.lock();
    promptDisplayed = val;
    promptCheckLock.unlock();
}

/*******************************************************************
 *
 *      ChatClient::CheckPromptDisplayed
 *
 *******************************************************************/
bool ChatClient::CheckPromptDisplayed()
{
    promptCheckLock.lock();
    bool retVal = promptDisplayed;
    promptCheckLock.unlock();

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
    for(int i=0; i<file.length(); i++)
        outStream.write(&file[i], 1);
    outStream.close();
}

/*******************************************************************
 *
 *      ChatClient::ReadFile
 *
 *******************************************************************/
string ChatClient::ReadFile(string filename)
{
    ifstream inStream (filename.c_str(), ios::in | ios::ate);
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
            DisplayMsg("Other Users Currently Connected: ", false);
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

    SetPromptDisplayed(false);

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
    static struct termios oldt, newt;
    struct timeval timeout;
    char inputChar = 0;
    int currentLength = 0;

    input = "";
    currentInput = "";
    DisplayPrompt();

    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

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
            /* Newline; input complete */
            if(inputChar == '\n')
            {
                cout << endl;
                flush(cout);
                SetPromptDisplayed(false);
                break;
            }
            /* Escape sequence; consume next 2 chars */
            else if(inputChar == 27)
            {
                read(STDIN_FILENO, &inputChar, 1);
                read(STDIN_FILENO, &inputChar, 1);
                continue;
            }
            /* Backspace */
            else if(inputChar == 0x7f)
            {
                if(currentLength > 0)
                {
                    currentLength--;
                    printf("\b");
                    printf(" ");
                    printf("\b");
                    flush(cout);
                    input = input.substr(0, input.length()-1);
                    displayLock.lock();
                    currentInput = currentInput.substr(0, currentInput.length()-1);
                    displayLock.unlock();
                    continue;
                }
                continue;
            }
            /* Grab char; echo to screen */
            else
            {
                currentLength++;
                input += inputChar;
                displayLock.lock();
                currentInput += inputChar;
                cout << inputChar;
                flush(cout);
                displayLock.unlock();
            }
        }
    }
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
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
            DisplayMsg("Error: file [" + filename + "] does not exist");
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
            DisplayMsg("Error: user [" + destination + "] not found");
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
            DisplayMsg("Error: user [" + destination + "] not found");
            return false;
        }
        filename = GetFilename(input);
        if(!FileExists(filename))
        {
            DisplayMsg("Error: file [" + filename + "] does not exist");
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
            DisplayMsg("Error: user [" + destination + "] not found");
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
            DisplayMsg("Error: user [" + destination + "] not found");
            return false;
        }
        filename = GetFilename(input);
        if(!FileExists(filename))
        {
            DisplayMsg("Error: file [" + filename + "] does not exist");
            return false;
        }
        filename = basename(filename.c_str());
        file = ReadFile(filename);
        output = string(BLKCST_FILE) + USER_TAG + destination + MSG_TAG + filename + MSG_TAG + file + MSG_END;
        return true;
    }

    DisplayMsg("Error: Command Not Found");
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
        display = "";
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
        currentMsg += string(msg, numBytes);
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
        display += source + " connnected";
        return;
    }

    // user disconnected
    if(CompareTag(tag, USER_DISCONNECT))
    {
        RemoveNeighbor(source);
        display += source + " disconnnected";
        return;
    }

    // message received
    if(CompareTag(tag, MSG_RCV))
    {
        msg = GetMsg(input);
        display += source + ": " + msg;
        return;
    }

    // file received
    if(CompareTag(tag, FILE_RCV))
    {
        filename = GetFilename(input);
        file = GetFile(input);
        CreateFile(filename, file);
        display += "File: [" + filename + "] sent by " + source;
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
    displayLock.lock();
    bool insertLineAbove = CheckPromptDisplayed();

    /* Carriage return and clear line, then display message */
    /* Redisplay prompt and current user input on next line */
    if(insertLineAbove)
    {
        promptCheckLock.lock();
        printf("\r%s\033[K\n", msg.c_str());
        fflush(stdout);
        printf("%s%s", prompt.c_str(), currentInput.c_str());
        fflush(stdout);
        promptCheckLock.unlock();
    }
    /* Prompt is not displayed, just display message */
    else
    {
        cout << msg;
        if(newline)
            cout << endl;
        else
            flush(cout);
    }

    displayLock.unlock();
}

/*******************************************************************
 *
 *      ChatClient::DisplayPrompt
 *
 *******************************************************************/
void ChatClient::DisplayPrompt()
{
    displayLock.lock();
    cout << prompt;
    flush(cout);
    SetPromptDisplayed(true);
    displayLock.unlock();
}
