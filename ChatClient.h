/*******************************************************************
 *      ChatClient.h
 *
 *      Author: William Posey
 *      Course: CNT5106C
 *
 *      Header file for Client in Internet Chat Application
 *******************************************************************/
#ifndef CHAT_CLIENT_H
#define CHAT_CLIENT_H

#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <termios.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>

#define BUFFER_LENGTH           100

using namespace std;

class ChatClient
{
public:
    ChatClient(char* clientUsername, char* port);
    void StartClient();

private:
    /* Methods to establish/tear down connection with server */
    bool ConnectToServer();
    void WaitForExit();
    void TerminateThreads();

    /* Methods to manage other user information */
    bool GetNextNeighbor(string neighbors, string& name);
    void AddNeighbor(string name);
    void RemoveNeighbor(string name);

    /* Methods used to retreive messages from cmd line and send to server */
    void ClientSend();
    void GetInput    (string& input);
    bool ParseInput  (string& input, string& output);
    void SendMsg     (string msg);

    /* Methods use to receive and parse messages from server */
    void ClientRecv();
    string ReceiveFromServer();
    void ParseMessage(string input, string &display);

    /* Methods used to parse send/recv messages */
    string GetTag(string msg);
    string GetCmd(string input);
    string GetDestination(string msg);
    string GetSource(string msg);
    string GetFilename(string msg);
    string GetMsg(string msg);
    string GetFile(string msg);
    void CreateFile(string filename, string file);
    string ReadFile(string filename);

    /* Methods to Check Send/Recv Information */
    bool CompareTag(string tag, string checkTag);
    bool CompareCmd(string cmd, string checkCmd);
    bool UserExists(string user);
    bool FileExists(string filename);

    /* Methods used to synchronize and display output */
    void SetPromptDisplayed(bool val);
    bool CheckPromptDisplayed();
    void DisplayMsg(string msg, bool newline = true);
    void DisplayPrompt();

    /* Server Information */
    string serverPortStr;
    int serverPort;
    int socket_fd;
    sockaddr_in serverAddr;
    bool serverShutdown;

    /* Display Information and Mutex Locks */
    bool promptDisplayed;
    string prompt;
    mutex promptCheckLock;
    mutex displayLock;
    string currentInput;

    /* Client and Other User Information */
    string username;
    vector<string> otherUsers;
    bool clientExiting;

    /* Received Msg Information */
    string recvMsgSource;
    char recvMsgType;
    int srcEnd;
    int filenameEnd;
    string filename;

    /* Thread information */
    thread SendThread;
    pthread_t send_t;
    thread RecvThread;
    pthread_t recv_t;
};

/* Atmoic variable to signify that the signal handler */
/* function has run and SIGINT was the signal */
volatile sig_atomic_t signalDetected;

/* Signal Handler Function; used to shutdown server when ctrl+C entered at cmd line */
void signalHandler(int sigNum)
{
    if(sigNum == SIGINT || sigNum == SIGABRT)
    {
        signalDetected = 1;
        cout << endl;
    }
}

static void restoreTerminal(void* oldTerm)
{
    struct termios* oldT = (struct termios*)oldTerm;
    tcsetattr( STDIN_FILENO, TCSANOW, oldT);
}

#endif //CHAT_CLIENT_H
