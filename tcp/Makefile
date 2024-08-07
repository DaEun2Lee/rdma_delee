# Makefile

CC = gcc
CFLAGS = -g
SERVER_SRC = tcp_server.c
CLIENT_SRC = tcp_client.c
SERVER_EXEC = server
CLIENT_EXEC = client

.PHONY: all clean run_server run_client

all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $<

$(CLIENT_EXEC): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $<

run_server: $(SERVER_EXEC)
	./$(SERVER_EXEC)

run_client: $(CLIENT_EXEC)
	./$(CLIENT_EXEC)

clean:
	rm -f $(SERVER_EXEC) $(CLIENT_EXEC)
