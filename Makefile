CC=gcc
CLIENT_SRC=source/client/
CLIENT_OUT=build/
CLIENT_FN=client
SERVER_SRC=source/server/
SERVER_OUT=build/
SERVER_FN=server

DEPS=-pthread
FLAGS=-g

server: $(SERVER_SRC)main.c
	$(CC) $(FLAGS) $(SERVER_SRC)main.c -o $(SERVER_OUT)$(SERVER_FN) $(DEPS)
client: $(CLIENT_SRC)main.c
	$(CC) $(FLAGS) $(CLIENT_SRC)main.c -o $(CLIENT_OUT)$(CLIENT_FN) $(DEPS)
run-server:
	valgrind --leak-check=full ./$(SERVER_OUT)$(SERVER_FN) 8080
run-client:
	valgrind --leak-check=full ./$(CLIENT_OUT)$(CLIENT_FN) 127.0.0.1 8080
