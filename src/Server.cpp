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
#include <unordered_map>
#include "./RedisParser.hpp"

#define BUFFER_SIZE 1024

std::unordered_map<std::string, std::string> data_store;

void handle(int fd)
{ 
  char buff[BUFFER_SIZE] = "";
  while(1)
  { 
    memset(&buff, '\0', sizeof(buff));
    int recv_bytes = recv(fd, buff, sizeof(buff), 0);
    if(recv_bytes <= 0)
      return;
    
    std::string recv_str(buff, recv_bytes);
    Redisparser request(recv_str);
    std::vector<std::string> argStr = request.parser();
    
    if(argStr.empty())
      continue;

    std::string reply;
    std::string command = argStr[0];
    
    if(command == "ping")
    {
      reply = "+PONG\r\n";
    }
    else if(command == "echo" && argStr.size() > 1)
    { 
      reply = "$" + std::to_string(argStr[1].size()) + "\r\n" + argStr[1] + "\r\n";
    }
    else if(command == "set" && argStr.size() >= 3)
    {
      data_store[argStr[1]] = argStr[2];
      reply = "+OK\r\n";
    }
    else if(command == "get" && argStr.size() == 2)
    {
      auto it = data_store.find(argStr[1]);
      if(it != data_store.end())
      {
        reply = "$" + std::to_string(it->second.size()) + "\r\n" + it->second + "\r\n";
      }
      else
      {
        reply = "$-1\r\n";
      }
    }
    else
    {
      reply = "-ERR unknown command\r\n";
    }

    send(fd, reply.c_str(), reply.size(), 0);
  }
}

int main(int argc, char **argv) {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   std::cerr << "Failed to create server socket\n";
   return 1;
  }

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
  std::vector<std::thread> cli_threads;
  
  while(1)
  {
    int clientFd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client connected\n";
    cli_threads.push_back(std::thread(handle, clientFd));
  }

  for(auto& cli_thread: cli_threads)
  {
    if(cli_thread.joinable())
      cli_thread.join();
  }

  close(server_fd);
  return 0;
}