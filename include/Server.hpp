#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include "ServerInstance.hpp"
#include <sys/select.h>
#include "HttpRequest.hpp"
#include "ConfigParser.hpp"

class Server {
public:
    Server();
    ~Server();

    void addInstance(ServerInstance* instance, const ServerConfig& config);
    void setServers(const std::vector<ServerConfig>& servers);
    void run();

private:
    std::vector<ServerInstance*> _instances;
    std::vector<ServerConfig> _servers;
    fd_set _master_set;
    fd_set _working_set;
    int _max_fd;

    void _initializeSets();
    void _handleNewConnection(int listen_fd);
    void _handleClientData(int client_fd);
    
    // Nuovi metodi per rispondere
    const LocationConfig* _findLocationMatch(const std::string& uri, const ServerConfig& server) const;
    std::string _getFilePath(const std::string& uri, const LocationConfig* location);
    void _handleGetRequest(int client_fd, const HttpRequest& request);
    void _handleHeadRequest(int client_fd, const HttpRequest& request);
    void _sendFile(int client_fd, const std::string& path);
    void _sendAutoindex(int client_fd, const std::string& path, const std::string& uri);
    void _sendNotFound(int client_fd, const std::string& uri);
    void _sendForbidden(int client_fd, const std::string& uri);
    void _sendError(int client_fd, int statusCode, const std::string& statusText, const std::string& message);
    void _handlePostRequest(int client_fd, const HttpRequest& request);
    void _sendPostResponse(int client_fd, const HttpRequest& request);
    void _handleDeleteRequest(int client_fd, const HttpRequest& request);
    void _sendDeleteResponse(int client_fd, const HttpRequest& request, bool success, const std::string& message);
    void _sendHeadResponse(int client_fd, int statusCode, const std::string& contentType, size_t contentLength);
    void _sendHeadError(int client_fd, int statusCode, const std::string& statusText);
};

#endif