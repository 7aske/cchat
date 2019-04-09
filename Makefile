CC=gcc
CLIENT_SRC=source/
CLIENT_OUT=out/
CLIENT_FN=client
SERVER_SRC=source/
SERVER_OUT=out/
SERVER_FN=server

DEPS=-pthread
FLAGS=-g

default_target: all

all: $(SERVER_SRC)server.c $(CLIENT_SRC)client.c
	$(CC) $(FLAGS) $(SERVER_SRC)server.c -o $(SERVER_OUT)$(SERVER_FN) $(DEPS)
	$(CC) $(FLAGS) $(CLIENT_SRC)client.c -o $(CLIENT_OUT)$(CLIENT_FN) $(DEPS)

server: $(SERVER_SRC)server.c
	$(CC) $(FLAGS) $(SERVER_SRC)server.c -o $(SERVER_OUT)$(SERVER_FN) $(DEPS)

client: $(CLIENT_SRC)client.c
	$(CC) $(FLAGS) $(CLIENT_SRC)client.c -o $(CLIENT_OUT)$(CLIENT_FN) $(DEPS)

val-server:
	valgrind --leak-check=full ./$(SERVER_OUT)$(SERVER_FN) 8080

val-client:
	valgrind --leak-check=full ./$(CLIENT_OUT)$(CLIENT_FN) 127.0.0.1 8080

run-server:
	./$(SERVER_OUT)$(SERVER_FN) 8080
run-client:
	./$(CLIENT_OUT)$(CLIENT_FN) 127.0.0.1 8080
