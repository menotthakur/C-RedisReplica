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

std::unordered_map<std::string, std::string> data_store;

std::vector<std::string> parse_resp(const std::string& input) {
    std::vector<std::string> result;
    size_t pos = 0;
    while (pos < input.size()) {
        if (input[pos] == '*') {
            // Array, skip the count
            pos = input.find("\r\n", pos) + 2;
        } else if (input[pos] == '$') {
            // Bulk string
            size_t len_end = input.find("\r\n", pos);
            int len = std::stoi(input.substr(pos + 1, len_end - pos - 1));
            pos = len_end + 2;
            result.push_back(input.substr(pos, len));
            pos += len + 2; // Skip the string and the \r\n
        } else {
            // Unexpected character
            break;
        }
    }
    return result;
}

std::string handle_command(const std::vector<std::string>& parts) {
    if (parts.empty()) {
        return "-ERR empty command\r\n";
    }

    if (parts[0] == "PING") {
        return "+PONG\r\n";
    } else if (parts[0] == "ECHO" && parts.size() >= 2) {
        return "$" + std::to_string(parts[1].length()) + "\r\n" + parts[1] + "\r\n";
    } else if (parts[0] == "SET" && parts.size() >= 3) {
        data_store[parts[1]] = parts[2];
        return "+OK\r\n";
    } else if (parts[0] == "GET" && parts.size() == 2) {
        auto it = data_store.find(parts[1]);
        if (it != data_store.end()) {
            return "$" + std::to_string(it->second.length()) + "\r\n" + it->second + "\r\n";
        } else {
            return "$-1\r\n";
        }
    }

    return "-ERR unknown command\r\n";
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
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

    std::cout << "Waiting for a client to connect...\n";

    while (true) {
        struct sockaddr_in client_addr;
        int client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
        
        if (client_fd < 0) {
            std::cerr << "Failed to accept client connection\n";
            continue;
        }

        std::cout << "Client connected\n";

        char buffer[1024];
        while (true) {
            ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_read <= 0) {
                break;
            }
            buffer[bytes_read] = '\0';

            std::vector<std::string> command = parse_resp(std::string(buffer));
            std::string response = handle_command(command);

            if (send(client_fd, response.c_str(), response.length(), 0) == -1) {
                std::cerr << "Failed to send message\n";
                break;
            }
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}