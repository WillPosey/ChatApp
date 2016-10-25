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

/********* Messsage Codes *******/  /* Source, Description */
// setup
#define USERNAME_REQUEST    "ur"    // Server, requests client's username after establishing connection
#define USERNAME_TAKEN      "ut"    // Server, reponse to client's username submission; username accepted
#define OTHER_USERS         "ou"    // Server, response with all usernames

// client to server
#define SEND_MSG            "sm"    // Client, message to send to a single user
#define SEND_FILE           "sf"    // Client, file to send to a single user
#define BRDCST_MSG          "bm"    // Client, message to send to all users
#define BRDCST_FILE         "bf"    // Client, file to send to all users
#define BLKCST_MSG          "km"    // Client, message to send to all users except one
#define BLKCST_FILE         "kf"    // Client, file to send to all users except one
#define CLIENT_SHUTDOWN     "ex"    // Client, client message to server that it is disconnecting

// server to client
#define MSG_RCV             "rm"    // Server, message sent to a user
#define FILE_RCV            "rf"    // Server, file sent to a user
#define USER_CONNECT        "cn"    // Server, broadcast that a new user connected
#define USER_DISCONNECT     "dc"    // Server, broadcast that a user has disconnected
#define SERVER_SHUTDOWN     "sh"    // Server, server broadcast indicating it is shutting down
/********* End Messsage Codes *********/

/* Delimiters */
#define USER_TAG            '@'
#define MSG_TAG             '\"'
#define MSG_END             '\n'

/* Local Commands for Clients */
#define EXIT_CMD            "exit"
#define SEND_CMD            "send message "
#define SEND_FILE_CMD       "send file "
#define BRDCST_CMD          "broadcast message "
#define BRDCST_FILE_CMD     "broadcast file "
#define BLKCST_CMD          "blockcast message "
#define BLKCST_FILE_CMD     "blockcast file "

#endif //MESSAGE_DEFS_H
