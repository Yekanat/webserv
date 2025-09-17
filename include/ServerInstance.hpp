#ifndef SERVER_INSTANCE_HPP
#define SERVER_INSTANCE_HPP

#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>

class ServerInstance {
    public:
        ServerInstance(const std::string& host, int port);
        ~ServerInstance();

        int getSocket() const;
        void acceptClient();
        void handleClient(int client_fd);

    private:
        std::string _host;
        int _port;
        int _sockfd;
        struct sockaddr_in _addr;
};

#endif
