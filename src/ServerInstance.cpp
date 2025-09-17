#include "ServerInstance.hpp"
#include <cstring>

ServerInstance::ServerInstance(const std::string& host, int port)
    : _host(host), _port(port), _sockfd(-1) 
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0)
        throw std::runtime_error("Errore: socket() fallita");

    int opt = 1;
    setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(_port);
    _addr.sin_addr.s_addr = inet_addr(_host.c_str());

    if (bind(_sockfd, (struct sockaddr*)&_addr, sizeof(_addr)) < 0)
        throw std::runtime_error("Errore: bind() fallita");

    if (listen(_sockfd, 10) < 0)
        throw std::runtime_error("Errore: listen() fallita");

    std::cout << "Socket in ascolto su " << _host << ":" << _port << std::endl;
}

ServerInstance::~ServerInstance() {
    if (_sockfd >= 0)
        close(_sockfd);
}

int ServerInstance::getSocket() const {
    return _sockfd;
}

void ServerInstance::acceptClient() {
    int client_fd = accept(_sockfd, NULL, NULL);
    if (client_fd < 0)
        throw std::runtime_error("Errore: accept() fallita");

    std::cout << "Nuovo client connesso (fd=" << client_fd << ")" << std::endl;
}

void ServerInstance::handleClient(int client_fd) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int bytes = recv(client_fd, buffer, sizeof(buffer)-1, 0);
    if (bytes <= 0) {
        std::cout << "Client disconnesso (fd=" << client_fd << ")" << std::endl;
        close(client_fd);
        return;
    }

    std::cout << "Richiesta ricevuta (fd=" << client_fd << "):\n" << buffer << std::endl;

    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "Hello World!";

    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
}
