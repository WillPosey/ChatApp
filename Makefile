#################################################################################################
#												#
#	Author: William Posey									#
#	Course: CNT5106										#
#	Makefile for Internet Chat Application							#
#												#
#################################################################################################
FLAGS		= -std=c++11 -g -Wall -Wno-unused-variable -Wno-unused-function -Wno-sign-compare
LINK		= -pthread
SERVER_SRC	= ChatServer.cpp
CLIENT_SRC	= ChatClient.cpp
SERVER_OBJ	= $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ	= $(CLIENT_SRC:.cpp=.o)
SERVER_TARGET	= server
CLIENT_TARGET	= client
CREATE_DIR	= directories
ALL_DIR		= Server Client1 Client2 Client3
COPY_DIR	= copy_directories

############################## TARGETS ##############################
all		: $(CLIENT_TARGET) $(SERVER_TARGET) $(CREATE_DIR)

############################## RULES ##############################
$(CLIENT_TARGET): $(CLIENT_OBJ)
		g++ $(LINK) $(FLAGS) -o $@ $(CLIENT_OBJ)

$(SERVER_TARGET): $(SERVER_OBJ)
		g++ $(LINK) $(FLAGS) -o $@ $(SERVER_OBJ)

$(CREATE_DIR)	:
		mkdir -p $(ALL_DIR)
		cp $(SERVER_TARGET) Server
		cp $(CLIENT_TARGET) Client1
		cp $(CLIENT_TARGET) Client2
		cp $(CLIENT_TARGET) Client3
	
.cpp.o		:
		g++ $(FLAGS) $(LINK) -c $< -o $@

############################## CLEAN ###################################
.PHONY		: clean

client_clean	:
		rm -f $(CLIENT_OBJ) $(CLIENT_TARGET)

server_clean	:
		rm -f $(SERVER_OBJ) $(SERVER_TARGET)

clean 		: 
		rm -f *.o $(SERVER_TARGET) $(CLIENT_TARGET)
		rm -r -f $(ALL_DIR)
