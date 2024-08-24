

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <vector>
#include "./RedisParser.hpp"
#define BUFFER_SIZE 128
void handle(int fd)
{ 
  char buff[BUFFER_SIZE] = "";
  bzero(&buff,sizeof(buff));
  while(1)
  { 
    memset(&buff,'\0',sizeof(buff));
    int recv_bytes = recv(fd, buff, sizeof(buff), 0);
    if(recv_bytes <= 0 )
      return ;
    std::string recv_str(buff ,recv_bytes);
    Redisparser request(recv_str);
    std::vector<std::string> argStr =  request.parser();
    // if(argStr.empty())
    // {
    //   send(fd, argStr[0].c_str() ,argStr[0].size(), 0);
    //   return ;
    // }
    std::string reply = argStr[0];
    if(reply == "ping")
    {
      send(fd, "+PONG\r\n",7, 0);
    }
    else if( reply == "echo" )
    { 
      for(int i = 1; i < argStr.size() ; ++ i)
        reply = "$"+ std::to_string(argStr[i].size()) + "\r\n" +argStr[i] + "\r\n";
        send(fd, reply.c_str() , reply.size(), 0);
    }
    else
    {
      break;
    }
     }
  return ;
}
int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  // std::cout << "Logs from your program will appear here!\n";
  // Uncomment this block to pass the first stage
  //
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }
  //
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
  //
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  std::cout << "Waiting for a client to connect...\n";
  std::vector<std::thread> cli_threads;
  while(1)
  {
    int clientFd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";
    cli_threads.push_back(std::thread(handle, clientFd) );
  }
  for(auto& cli_thread: cli_threads)
  {
    if(cli_thread.joinable())
      cli_thread.join();
  }
  close(server_fd);
  return 0;
}


/*
Potential Interview Questions:
Why did you introduce the Redis parser in your code?

Explanation: I introduced the Redis parser to handle more complex Redis commands like PING and ECHO. 
This modular approach allows the program to interpret and process different commands from the client in a structured manner.

How does your code handle multiple client connections?

Explanation: The code handles multiple client connections by spawning a new thread for each incoming client connection.

 Each thread runs the handle function, which processes the client's commands independently. 
 This ensures that the server can handle multiple clients simultaneously without blocking.
What is the purpose of the SO_REUSEADDR socket option?

Explanation: The SO_REUSEADDR socket option allows the server to bind to an address that is in a TIME_WAIT state.

 This is useful when restarting the server frequently, as it prevents the "Address already in use" error.

Can you explain the PING and ECHO command handling in your code?

Explanation: The PING command is handled by sending back a +PONG\r\n response to the client. 
The ECHO command takes additional arguments and sends them back to the client, prefixed with the length of each argument, formatted according to the Redis protocol.

Why do you use the std::vector<std::thread> for client threads?

Explanation: I used std::vector<std::thread> to store all the client-handling threads. 
This allows me to keep track of all threads and ensures that they can be properly joined before the server shuts down, avoiding potential resource leaks or crashes.
*/




/*
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <vector>

// Function to handle communication with a connected client
void handle_client(int client_fd) {
    // Message to send back to the client
    std::string msg = "+PONG\r\n";
    
    // Infinite loop to keep the connection with the client alive
    while (true) {
        char buf[1024];  // Buffer to store the client's message
        
        // Receive data from the client
        int rc = recv(client_fd, &buf, sizeof(buf), 0);
        
        // If the client has disconnected or an error occurred, exit the loop
        if (rc <= 0) {
            close(client_fd);  // Close the connection with the client
            break;
        }
        
        // Send a PONG response back to the client
        write(client_fd, msg.c_str(), msg.size());
    }
}

int main(int argc, char **argv) {
    // This is where the server setup begins
    std::cout << "Logs from your program will appear here!\n";

    // Create a socket using IPv4 (AF_INET) and TCP (SOCK_STREAM)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Check if the socket was successfully created
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket\n";
        return 1;
    }

    // Set the socket option to reuse the address (useful for quick restarts)
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed\n";
        return 1;
    }

    // Define the server's address and port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;  // Use IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Bind to any available network interface
    server_addr.sin_port = htons(6379);  // Use port 6379 (default Redis port)

    // Bind the socket to the specified address and port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 6379\n";
        return 1;
    }

    // Start listening for incoming connections, with a maximum backlog of 5 connections
    int connection_backlog = 5;
    if (listen(server_fd, connection_backlog) != 0) {
        std::cerr << "listen failed\n";
        return 1;
    }

    // Prepare to accept client connections
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    

    // Infinite loop to accept and handle multiple client connections
    while (true) {
        // Accept a new client connection
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
        std::cout << "Client connected\n";
        
        // Create a new thread to handle the client, allowing the server to accept more connections
        std::thread nw(handle_client, client_fd);
        
        // Detach the thread so that it can run independently
        nw.detach();
    }

    // Close the server socket when done (this line will never be reached in this infinite loop)
    close(server_fd);
    return 0;
}


*/



/*Potential Interview Questions and Explanations:
Q: Why did you decide to switch from the newer code to the older one?

A: The newer code is single-threaded, meaning it can only handle one client at a time. This limits its scalability in real-world
 scenarios where a server needs to manage multiple client connections simultaneously. The older code introduces threading, which 
 allows the server to handle multiple clients concurrently, making it more robust and suitable for production environments.
Q: What are the main differences between the new and old code?

A: The main difference is the use of threading in the old code to handle multiple clients at once. The newer code is simpler but
 only manages one client connection at a time, which can cause the server to become unresponsive if multiple clients try to connect. 
 The old code also makes use of a more robust buffer management approach and better handles socket closures.
Q: What potential issues did you identify in the newer code?

A: The newer code has a few potential issues:
It uses a fixed-size buffer of 100 bytes, which may not be sufficient for all message sizes, leading to data truncation.
The code is single-threaded, limiting it to one client at a time, which can be problematic in a multi-client environment.
The buffer is declared as char* msg[100], which might cause undefined behavior due to improper memory handling.
Q: Why did you include error handling in the newer code?

A: Error handling is crucial for identifying issues during socket communication, such as failed message sending or receiving. 
This ensures that the server can gracefully handle unexpected situations, like a client disconnecting or sending an incomplete message, 
which is essential for maintaining a reliable server.
Q: How does the older code improve upon the newer one?

A: The older code improves scalability by using threading, allowing the server to handle multiple clients concurrently. 
This makes the server more efficient and responsive in a real-world scenario where multiple clients might connect simultaneously. 
Additionally, the buffer management and client handling logic in the older code are more robust, reducing the risk of errors or crashes.*/









/*

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

int main(int argc, char **argv) {
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage
  //
     int server_fd = socket(AF_INET, SOCK_STREAM, 0);
     if (server_fd < 0) {
      std::cerr << "Failed to create server socket\n";
      return 1;
     }
  
  // // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }
 
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(6379);
 
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 6379\n";
    return 1;
  }
 
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }
 
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
 
  std::cout << "Waiting for a client to connect...\n";
 
  //accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len); //the return value of accept function stored in client_fd.holds the socket file descriptor of the newly accepted client connection.
  //send(client_fd, "+PONG\r\n", 7, 0);   // "+PONG\r\n": This is the message to be sent to the client. It's a string containing the characters "+PONG\r\n". The \r\n at the end is a carriage return and line feed, which is a common way to terminate a message in network communication. 
//  7: This is the length of the message to be sent, in bytes. In this case, the length of the string "+PONG\r\n" is 7 bytes:
//
//+ (1 byte)
//P (1 byte)
//O (1 byte)
//N (1 byte)
//G (1 byte)
//\r (1 byte)
//\n (1 byte)
//The send function will send exactly 7 bytes of data to the client.
//
//0: This is the flags argument to the send function. In this case, it's set to 0, which means no special flags are used. The send function will perform a normal,
// blocking send operation.
  
  
  std::cout << "Client connected\n";

  while (1) {
  char* msg[100];
  if (recv(client_fd, (void*) msg, 100, 0) == -1) {
    std::cerr << "Failed to receive message\n";
    return 1;
  }
  const char* message = "+PONG\r\n";
  if (send(client_fd, (const void*) message, strlen(message), 0) == -1) {
    std::cerr << "Failed to send message\n";
    return 1;
  }
  }
 
  close(server_fd);  
  return 0;
}


*/