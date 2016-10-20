/*******************************************************************
 *      MessageDefs.h
 *
 *      Author: William Posey
 *      Course: CNT5106C
 *
 *      This file contains defintions for client/server messages
 *******************************************************************/
#ifndef MESSAGE_DEFS_H
#define MESSAGE_DEFS_H

/* Messsage Codes */                /* Source, Description */
#define USERNAME_REQUEST    0       // Server, requests client's username after establishing connection
#define USERNAME_SUBMIT     1       // Client, response to server's request for username
#define USERNAME_VALID      2       // Server, reponse to client's username submission; username accepted
#define OTHER_USERS_REQUEST 3       // Client, request for all usernames
#define USERNAMES_REPLY     4       // Server, response with all usernames
#define USER_CONNECT        5       // Server, broadcast that a new user connected
#define USER_DISCONNECT     6       // Server, broadcast that a user has disconnected
#define SEND_MSG            7       // Client, message to send to a single user
#define SEND_FILE           8       // Client, file to send to a single user
#define BRDCST_MSG          9       // Client, message to send to all users
#define BRDCST_FILE         10      // Client, file to send to all users
#define BLKCST_MSG          11      // Client, message to send to all users except one
#define BLKCST_FILE         12      // Client, file to send to all users except one
#define MSG_RCV             13      // Server, message sent to a user
#define FILE_RCV            14      // Server, file sent to a user
#define SERVER_SHUTDOWN     15      // Server, server broadcast indicating it is shutting down
#define CLIENT_SHUTDOWN     16      // Client, client message to server that it is disconnecting

#define SERVER_HEARTBEAT    17      // Server, request to client to verify it is still alive
#define CLIENT_HEARTBEAT    18      // Client, response to server heartbeat

/* Delimiters */
#define MSG_END             '\n'
#define MSG_SRC_END         '\r'
#define MSG_DST_END         '\t'
#define FILENAME_END        '\f'
#define USERNAME_END        '\v'

/* Local Commands for Clients */
#define DISCONNECT_CMD      "disconnect"
#define EXIT_CMD            "exit"
#define SEND_CMD            "send"
#define SEND_FILE_CMD       "send-file"
#define BRDCST_CMD          "broadcast"
#define BRDCST_FILE_CMD     "broadcast-file"
#define BLKCST_CMD          "blockcast"
#define BLKCST_FILE_CMD     "blockcast-file"

#endif //MESSAGE_DEFS_H
