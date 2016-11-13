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
    bool ConnectToServer();
    bool GetNextNeighbor(string neighbors, string& name);
    void AddNeighbor(string name){otherUsers.push_back(name);}
    void RemoveNeighbor(string name);
    void WaitForExit();
    void TerminateThreads(){pthread_cancel(send_t); pthread_cancel(recv_t);}

    void ClientSend();
    void GetInput    (string& input);
    bool ParseInput  (string& input, string& output);
    void SendMsg     (string msg);

    void ClientRecv();
    string ReceiveFromServer();
    void ParseMessage(string input, string &display);

    string GetTag(string msg);
    string GetCmd(string input);
    string GetDestination(string msg);
    string GetSource(string msg);
    string GetFilename(string msg);
    string GetMsg(string msg);
    string GetFile(string msg);
    void CreateFile(string filename, string file);
    string ReadFile(string filename);

    bool CompareTag(string tag, string checkTag);
    bool CompareCmd(string cmd, string checkCmd);
    bool UserExists(string user);
    bool FileExists(string filename);

    void SetPromptDisplayed(bool val);
    bool CheckPromptDisplayed();

    void DisplayMsg(string msg, bool newline = true);
    void DisplayPrompt();

    string serverPortStr;
    int serverPort;

    bool promptDisplayed;

    string username;
    string prompt;
    string currentInput;

    string recvMsgSource;
    char recvMsgType;
    int srcEnd, filenameEnd;
    string filename;

    vector<string> otherUsers;

    bool clientExiting, serverShutdown;
    mutex displayLock, promptCheckLock;

    thread SendThread;
    thread RecvThread;

    int socket_fd;
    sockaddr_in serverAddr;

};

volatile sig_atomic_t signalDetected;

void signalHandler(int sigNum)
{
    if(sigNum == SIGINT)
    {
        signalDetected = 1;
        cout << endl;
    }
}

#endif //CHAT_CLIENT_H
