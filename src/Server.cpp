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
#include <sstream>
#include <algorithm>

#define BUFFER_SIZE 1024

std::unordered_map<std::string, std::string> data_store;

std::vector<std::string> parse_resp(const std::string& input) {
    std::vector<std::string> result;
    std::istringstream iss(input);
    std::string line;

    std::getline(iss, line);
    if (line[0] != '*') {
        return result;  // Invalid format
    }

    int array_length = std::stoi(line.substr(1));

    for (int i = 0; i < array_length; ++i) {
        std::getline(iss, line);
        if (line[0] != '$') {
            return {};  // Invalid format
        }

        int str_length = std::stoi(line.substr(1));
        std::getline(iss, line);
        if (line.length() != str_length) {
            return {};  // Invalid length
        }

        result.push_back(line);
    }

    return result;
}

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
        std::vector<std::string> argStr = parse_resp(recv_str);
        
        if(argStr.empty())
            continue;

        std::string reply;
        std::string command = argStr[0];
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);
        
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
            reply = "-ERR unknown command '" + command + "'\r\n";
        }

        send(fd, reply.c_str(), reply.size(), 0);
    }
}

int main() {
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
    
    std::cout << "Waiting for a client to connect...\n";
    
    while(1)
    {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        int clientFd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
        if (clientFd < 0) {
            std::cerr << "Failed to accept client connection\n";
            continue;
        }
        std::cout << "Client connected\n";
        handle(clientFd);
        close(clientFd);
    }

    close(server_fd);
    return 0;
}