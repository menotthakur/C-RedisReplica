#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <map>
#include <chrono>
void* handle_client(void* arg);
std::vector<std::string> parse_command(const std::string& command);
std::map<std::string, std::string> keyValues;
std::map<std::string, long> expTime;
int main(int argc, char **argv) {
  // Uncomment this block to pass the first stage
   int server_fd = socket(AF_INET, SOCK_STREAM, 0);
   if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
   }
   // Since the tester restarts your program quite often, setting SO_REUSEADDR
   // ensures that we don't run into 'Address already in use' errors
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
    while (true){
        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t*) &client_addr_len);
        if (client_fd < 0) {
            std::cerr << "accept failed\n";
            return 1;
        }
        pthread_t thread;
        pthread_create(&thread, nullptr, handle_client, &client_fd);
    }
  return 0;
}
void* handle_client(void* arg) {
    int clientSock = *(int*)arg;
    char buffer[1024];
    char recvBuffer[1024];
    strcpy(buffer, "+PONG\r\n");
    long n;
    while ((n=recv(clientSock, recvBuffer, 1024, 0)) > 0) {
      recvBuffer[n] = '\0';
      std::vector<std::string> command = parse_command(recvBuffer);
      std::cout<< "Command[0] " << command[0] << std::endl;
      if (strcmp(command[0].c_str(), "SET")==0){
            keyValues[command[1]] = command[2];
        if (command.size()>3 && strcmp(command[3].c_str(), "px")==0){
            auto now = std::chrono::system_clock::now();
            auto now_in_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
            auto value = now_in_ms.time_since_epoch();
            long current_time_in_ms = value.count();
            expTime[command[1]] = current_time_in_ms + std::stoi(command[4]);
        }
           else expTime[command[1]] = -1;
            strcpy(buffer, "+OK\r\n");
            send(clientSock, buffer, strlen(buffer), 0);
      }
      else if (strcmp(command[0].c_str(), "GET")==0){
            if (keyValues.find(command[1]) != keyValues.end()){
          auto now = std::chrono::system_clock::now();
          auto now_in_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
          auto value = now_in_ms.time_since_epoch();
          long current_time_in_ms = value.count();
          if (keyValues.find(command[1]) != keyValues.end() && (expTime[command[1]]==-1 || expTime[command[1]]>current_time_in_ms)){
                strcpy(buffer, "$");
                strcpy(buffer+1, std::to_string(keyValues[command[1]].length()).c_str());
                strcpy(buffer+1+std::to_string(keyValues[command[1]].length()).length(), "\r\n");
                strcpy(buffer+1+std::to_string(keyValues[command[1]].length()).length()+2, keyValues[command[1]].c_str());
                strcpy(buffer+1+std::to_string(keyValues[command[1]].length()).length()+2+keyValues[command[1]].length(), "\r\n");
                send(clientSock, buffer, strlen(buffer), 0);
            }
            else{
                strcpy(buffer, "$-1\r\n");
                send(clientSock, buffer, strlen(buffer), 0);
            }
      }
      else if (strcmp(command[0].c_str(), "ECHO")==0){
        int i = 1;
        std::string bulkString = "$";
        while (i<command.size())
        {
            bulkString += std::to_string(command[i].length());
            bulkString += "\r\n";
            bulkString += command[i];
            std::cout<<"Command["<<i<<"]: " << command[i] << std::endl;
            bulkString += "\r\n";
            i++;
        }
        // Now bulkString contains the entire message to be sent
        // You can return it or use it elsewhere
          strcpy(buffer, bulkString.c_str());
          send(clientSock, buffer, strlen(buffer), 0);
      }
      else if (strcmp(command[0].c_str(), "PING")==0){
          strcpy(buffer, "+PONG\r\n");
          send(clientSock, buffer, strlen(buffer), 0);
      }
    }
    close(clientSock);
    return nullptr;
}
std::vector<std::string> parse_command(const std::string& command) {
    long i = 1;
    unsigned long len = 0;
    int arrLen = 0;
    std::vector<std::string> result;
    switch (command[0]) {
        case '$':
            while (command[i]!='\0'){
                while (command[i] != '\r'&&command[i+1]!='\n') {
                    len = len*10 + (command[i] - '0');
                    i++;
                }
                result.push_back(command.substr(i+2, len));
                i = i+2+len+2;
                len = 0;
            }
            break;
        case '*':
            i = 1;
            while (command[i] != '\r'&&command[i+1]!='\n') {
                arrLen = arrLen*10 + (command[i] - '0');
                i++;
            }
            i = i+2;
            while(arrLen>0){
                len = 0;
                i++;//skips the $ or the + character
                while (command[i] != '\r') {
                    len = len*10 + (command[i] - '0');
                    i++;
                }
                result.push_back(command.substr(i+2, len));
                i = i+2+len+2;
                arrLen--;
            }
            break;
        default:
            len = 0;
            len = command.length();
            result.push_back(command.substr(1, len-3));
    }
    return result;
}