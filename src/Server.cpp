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
    recv(fd, buff, 15, 0);
    if(strcasecmp(buff, "exit") == 0)
  
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
    if(strcasecmp(buff,"*1\r\n$4\r\nping\r\n") !=0 )
    {  
      bzero(&buff,sizeof(buff));
      continue;
    }
    send(fd, "+PONG\r\n",7, 0);
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