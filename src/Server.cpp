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
        std::cerr << "Invalid format: expected '*', got '" << line << "'" << std::endl;
        return result;  // Invalid format
    }

    int array_length = std::stoi(line.substr(1));
    std::cerr << "Array length: " << array_length << std::endl;

    for (int i = 0; i < array_length; ++i) {
    std::getline(iss, line);
    if (line[0] != '$') {
        std::cerr << "Invalid format: expected '$', got '" << line << "'" << std::endl;
        return {};  // Invalid format
    }

    int str_length = std::stoi(line.substr(1));
    std::getline(iss, line);
    if (line.length() != str_length) {
        std::cerr << "Invalid length: expected " << str_length << ", got " << line.length() << std::endl;
        return {};  // Invalid length
    }

    result.push_back(line);
    std::cerr << "Parsed argument: " << line << std::endl;
}

// Add this check
if (result.size() != array_length) {
    std::cerr << "Invalid array length: expected " << array_length << ", got " << result.size() << std::endl;
    return {};  // Invalid array length
}

    return result;
}

void handle_client(int client_fd) {
    char buff[BUFFER_SIZE];
    while (true) {
        memset(buff, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_fd, buff, BUFFER_SIZE - 1, 0);
        
        if (bytes_received <= 0) {
            std::cerr << "Client disconnected or error occurred" << std::endl;
            break;
        }

        std::string received_data(buff, bytes_received);
        std::cerr << "Received data: " << received_data << std::endl;

        std::vector<std::string> args = parse_resp(received_data);
        if (args.empty()) {
            std::cerr << "Failed to parse RESP" << std::endl;
            continue;
        }

        std::string command = args[0];
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);

        std::string response;
        if (command == "ping") {
            response = "+PONG\r\n";
        } else if (command == "echo" && args.size() > 1) {
            response = "$" + std::to_string(args[1].length()) + "\r\n" + args[1] + "\r\n";
        } else if (command == "set" && args.size() >= 3) {
            data_store[args[1]] = args[2];
            response = "+OK\r\n";
        } else if (command == "get" && args.size() == 2) {
            auto it = data_store.find(args[1]);
            if (it != data_store.end()) {
                response = "$" + std::to_string(it->second.length()) + "\r\n" + it->second + "\r\n";
            } else {
                response = "$-1\r\n";
            }
        } else {
            response = "-ERR unknown command '" + command + "'\r\n";
        }

        std::cerr << "Sending response: " << response << std::endl;
        ssize_t bytes_sent = send(client_fd, response.c_str(), response.length(), 0);
        if (bytes_sent <= 0) {
            std::cerr << "Failed to send response" << std::endl;
            break;
        }
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create server socket" << std::endl;
        return 1;
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt failed" << std::endl;
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(6379);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        std::cerr << "Failed to bind to port 6379" << std::endl;
        return 1;
    }

    if (listen(server_fd, 5) != 0) {
        std::cerr << "listen failed" << std::endl;
        return 1;
    }

    std::cout << "Server listening on port 6379" << std::endl;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (client_fd < 0) {
            std::cerr << "Failed to accept client connection" << std::endl;
            continue;
        }

        std::cout << "New client connected" << std::endl;
        handle_client(client_fd);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}