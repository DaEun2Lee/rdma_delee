# README.md

## Project Overview

This directory contains the following files:
- **test_server.c**: socket server
- **test_client.c**: socket client
- **test_socket.h**: both the socket server and client implemented with threads
- **test_socket.c**: both the socket server and client implemented with threads
- **simple_socket_send.h**: sends requests to the socket server

### How to Run

1. Ensure you have the necessary permissions to execute the script.
2. Run the script using the following command:
   ```sh
   # Compile the project
   make

   # Start the server
   ./server

   # Start the client
   ./client
