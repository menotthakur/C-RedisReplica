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
#include <pthread.h>
#include <cassert>
#include <map>
const int BUFFER_SIZE = 2048;
int head = 0;
std::string data;
std::map<std::string, std::string> storage;
std::map<std::string, std::chrono::time_point<std::chrono::system_clock>> expiry;
void err(std::string msg) {
  std::cerr << "ERROR: " << msg << std::endl;
}
char nextToken() {
  if (head == data.size()) return '\0';
  return data[head++];
}
void match(char c) {
  char token = nextToken();
  if (c != token) err("Expected " + std::string(1, c) + " , found " + std::string(1, token));
}
std::string parseBulkString() {
  int len = 0;
  while (1) {
    char c = nextToken();
    if (c >= '0' && c <= '9') {
      len = len * 10 + c - '0';
    } else break;
  }
  match('\n');
  std::string msg;
  for (int i = 0; i < len; ++i) {
    msg += nextToken();
  }
  match('\r');
  match('\n');
  return msg;
}
std::vector<std::string> parseCommand() {
  std::vector<std::string> cmds;
  head = 0;
  match('*');
  int argc = 0;
  while (1) {
    char c = nextToken();
    if (c >= '0' && c <= '9') {
      argc = argc * 10 + c - '0';
    } else break;
  }
  match('\n');
  for (int i = 0; i < argc; ++i) {
    match('$');
    cmds.push_back(parseBulkString());  
  }
  return cmds;
}
void handleRequest(int client_fd) {
  char buffer[BUFFER_SIZE] = "";
  std::cout << "Handling request from client " << client_fd << "\n";
  while (true) {
    while (recv(client_fd, buffer, BUFFER_SIZE, 0)) {
      std::string request(buffer), response;
      data = request;
      std::vector<std::string> cmds = parseCommand();
      if (cmds[0] == "ping") {
        response = "+PONG\r\n";
      } else if (cmds[0] == "echo") {
        std::string argv = cmds[1];
        response = "$" + std::to_string(argv.size()) + "\r\n" + argv + "\r\n";      
      } else if (cmds[0] == "get") {
        std::string key = cmds[1];
        //if (storage.find(key) == storage.end()) {
        if (storage.find(key) == storage.end() || expiry[key] < std::chrono::system_clock::now()) {
          response = "$-1\r\n";
        } else {
          std::string value = storage[key];
          response = "$" + std::to_string(value.size()) + "\r\n" + value + "\r\n";
        }    
      } else if (cmds[0] == "set") {
        std::chrono::time_point<std::chrono::system_clock> exp;
        std::string key = cmds[1];
        std::string value = cmds[2];
        if (cmds.size() == 5) {
          int ttl = std::stoi(cmds[4]);
          exp = std::chrono::system_clock::now() + std::chrono::milliseconds(ttl);
        } else {
          exp = std::chrono::system_clock::now() + std::chrono::seconds(1000);
        }
        storage[key] = value;
        expiry[key] = exp;
        response = "$2\r\nOK\r\n";
      }
      send(client_fd, response.c_str(), response.size(), 0);
    }
  }
}
int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";
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
  while (true) {
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    if (client_fd == -1) return 1;
    std::cout << "Client connected\n";
    pid_t pid = fork();
    if (pid == 0) {
      handleRequest(client_fd);
      close(client_fd);
      exit(0);
    } else {
      close(client_fd);
    }
  }
  close(server_fd);
  return 0;
}