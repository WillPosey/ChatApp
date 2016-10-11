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

    void ClientSend();
    int GetInput    (string input, string& output){return PARSE_INPUT;};
    int ParseInput  (string input, string& output){return SEND_INPUT;};
    int SendInput   (string input, string& output){return GET_INPUT;};
    int ErrorInput  (string input, string& output){return GET_INPUT;};

    void ClientRecv();
    int GetMsgType  (string msg, string& display){display = msg; display += ":1"; cout << display << endl; return 1;};
    int GetSource   (string msg, string& display){display += ":2"; cout << display << endl; return 2;};
    int GetMsg      (string msg, string& display){display += ":3"; cout << display << endl; return 3;};
    int GetFilename (string msg, string& display){display += ":4"; cout << display << endl; return 4;};
    int GetFile     (string msg, string& display){display += ":5"; cout << display << endl; return 5;};
    int Reply       (string msg, string& display){display += ":6"; cout << display << endl; return 6;};
    int DisplayMsg  (string msg, string& display){display += ":7"; cout << display << endl; return 7;};

    void SetExit(bool val);
    bool CheckExit();
    void SetShutdown(bool val);
    bool CheckShutdown();

private:
    typedef enum
    {
        SERVER_INFO,
        CONNECT,
        USERNAME,
        GET_USERS,
        START_THREADS,
        COMPLETE
    } SetupCodes;

    typedef enum
    {
        GET_INPUT,
        PARSE_INPUT,
        SEND_INPUT,
        ERROR_INPUT
    } SendCodes;

    typedef enum
    {
        GET_TYPE,
        GET_SRC,
        GET_MSG,
        GET_FILENAME,
        GET_FILE,
        REPLY,
        DISPLAY,
        PARSE_COMPLETE
    } RecvCodes;

    int (ChatClient::*setupStateFunction[5])() = {  &ChatClient::GetServerInfo,
                                                    &ChatClient::ConnectToServer,
                                                    &ChatClient::GetUsername,
                                                    &ChatClient::GetOtherUsers,
                                                    &ChatClient::SetupComplete };

    int (ChatClient::*sendStateFunction[4])(string input, string& output) = {   &ChatClient::GetInput,
                                                                                &ChatClient::ParseInput,
                                                                                &ChatClient::SendInput,
                                                                                &ChatClient::ErrorInput };

    int (ChatClient::*recvStateFunction[7])(string msg, string& display) = {    &ChatClient::GetMsgType,
                                                                                &ChatClient::GetSource,
                                                                                &ChatClient::GetMsg,
                                                                                &ChatClient::GetFilename,
                                                                                &ChatClient::GetFile,
                                                                                &ChatClient::Reply,
                                                                                &ChatClient::DisplayMsg };
    string serverPortStr;
    int serverPort;
    string username;
    vector<string> otherUsers;

    bool exiting, serverExit;
    pthread_mutex_t exitLock, shutdownLock, displayLock;

    thread SendThread;
    thread RecvThread;

    int socket_fd;
    sockaddr_in serverAddr;

};

#endif //CHAT_CLIENT_H
