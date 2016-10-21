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
#include <string>
#include <vector>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>

#define BUFFER_LENGTH           100

using namespace std;

class ChatClient
{
public:
    ChatClient();
    ~ChatClient();

    void StartClient();
    int GetServerInfo();
    int ConnectToServer();
    int GetUsername();
    int GetOtherUsers();
    int SetupComplete();
    int WaitForExit();

    void ClientSend();
    int GetInput    (string& input, string& output);
    int ParseInput  (string& input, string& output);
    int SendMsg     (string& input, string& output);
    int ErrorInput  (string& input, string& output);

    void ClientRecv();
    bool ReceiveFromServer(string& recvMsg);
    int GetMsgType      (string msg, string& display);
    int GetSource       (string msg, string& display);
    int GetMsg          (string msg, string& display);
    int GetFilename     (string msg, string& display);
    int GetFile         (string msg, string& display);
    int DisplayRecvMsg  (string msg, string& display);

    void SetExit(bool val);
    bool CheckExit();
    void SetDisconnect(bool val);
    bool CheckDisconnect();
    void SetShutdown(bool val);
    bool CheckShutdown();
    void SetPromptDisplayed(bool val);
    bool CheckPromptDisplayed();

    void DisplayMsg(string msg, bool newline = true);
    void DisplayPrompt();

private:
    typedef enum
    {
        SERVER_INFO,
        CONNECT,
        USERNAME,
        GET_USERS,
        START_THREADS,
        COMPLETE,
        CLIENT_EXIT
    } SetupCodes;

    typedef enum
    {
        GET_INPUT,
        PARSE_INPUT,
        SEND_INPUT,
        END_THREAD
    } SendCodes;

    typedef enum
    {
        GET_TYPE,
        GET_SRC,
        GET_MSG,
        GET_FILENAME,
        GET_FILE,
        DISPLAY,
        PARSE_COMPLETE
    } RecvCodes;

    int (ChatClient::*setupStateFunction[6])() = {  &ChatClient::GetServerInfo,
                                                    &ChatClient::ConnectToServer,
                                                    &ChatClient::GetUsername,
                                                    &ChatClient::GetOtherUsers,
                                                    &ChatClient::SetupComplete,
                                                    &ChatClient::WaitForExit };

    int (ChatClient::*sendStateFunction[3])(string& input, string& output) = {  &ChatClient::GetInput,
                                                                                &ChatClient::ParseInput,
                                                                                &ChatClient::SendMsg };

    int (ChatClient::*recvStateFunction[6])(string msg, string& display) = {    &ChatClient::GetMsgType,
                                                                                &ChatClient::GetSource,
                                                                                &ChatClient::GetMsg,
                                                                                &ChatClient::GetFilename,
                                                                                &ChatClient::GetFile,
                                                                                &ChatClient::DisplayRecvMsg };
    string serverPortStr;
    int serverPort;

    bool promptDisplayed;

    string username;

    string recvMsgSource;
    char recvMsgType;
    int srcEnd, filenameEnd;
    string filename;

    vector<string> otherUsers;

    bool exiting, serverExit, disconnect;
    pthread_mutex_t exitLock, shutdownLock, disconnectLock, displayLock, promptCheckLock;

    thread SendThread;
    thread RecvThread;

    int socket_fd;
    sockaddr_in serverAddr;

};

#endif //CHAT_CLIENT_H
