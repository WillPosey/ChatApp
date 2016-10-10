#####################################################
#
#	Author: William Posey
#	Course: CNT5106	
#	Makefile for Internet Chat Application
#	
#####################################################
FLAGS		= -std=c++11 -g -Wall -Wno-unused-variable -Wno-unused-function
LINK		= 
SERVER_SRC	= ChatServer.cpp
CLIENT_SRC	= ChatClient.cpp
SERVER_OBJ	= $(SERVER_SRC:.cpp=.o)
CLIENT_OBJ	= $(CLIENT_SRC:.cpp=.o)
SERVER_TARGET	= ChatServer
CLIENT_TARGET	= ChatClient

all		: $(CLIENT_TARGET) $(SERVER_TARGET)
client		: $(CLIENT_TARGET)
server		: $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJ)
		g++ $(LINK) $(FLAGS) -o $@ $(CLIENT_OBJ)

$(SERVER_TARGET): $(SERVER_OBJ)
		g++ $(LINK) $(FLAGS) -o $@ $(SERVER_OBJ)
			
.cpp.o		:
		g++ $(FLAGS) -c $< -o $@

.PHONY		: clean

clean 		: 
		rm *.o ChatServer ChatClient
