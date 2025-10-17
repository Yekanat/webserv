#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include "utils.hpp"
#include "HttpResponse.hpp"
#include <sys/stat.h>

Server::Server() : _max_fd(0) {
    FD_ZERO(&_master_set);
    FD_ZERO(&_working_set);
}

Server::~Server() {
    // Gli instances sono gestiti esternamente, non dobbiamo cancellarli qui
    
    // Chiudi tutti i socket aperti nel master set
    for (int fd = 0; fd <= _max_fd; ++fd) {
        if (FD_ISSET(fd, &_master_set)) {
            close(fd);
        }
    }
}

void Server::addInstance(ServerInstance* instance, const ServerConfig& config) {
    _instances.push_back(instance);
    
    // Verifica se questa configurazione è già presente
    bool found = false;
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i].server_name == config.server_name) {
            found = true;
            break;
        }
    }
    
    // Aggiungi solo se non è già presente
    if (!found) {
        _servers.push_back(config);
    }
}

void Server::setServers(const std::vector<ServerConfig>& servers) {
    _servers = servers;
}

void Server::_initializeSets() {
    FD_ZERO(&_master_set);
    _max_fd = 0;
    
    // Aggiungi tutti i socket di ascolto al master set
    for (size_t i = 0; i < _instances.size(); ++i) {
        int sockfd = _instances[i]->getSocket();
        FD_SET(sockfd, &_master_set);
        if (sockfd > _max_fd)
            _max_fd = sockfd;
    }
}

void Server::run() {
    _initializeSets();

    std::cout << "Server in esecuzione, in attesa di connessioni..." << std::endl;

    while (1) {
        _working_set = _master_set;  // Copia il master set

        // Attendi attività sui socket
        if (select(_max_fd + 1, &_working_set, NULL, NULL, NULL) < 0) {
            std::cerr << "select() fallita" << std::endl;
            break;
        }

        // Controlla tutti i socket per attività
        for (int i = 0; i <= _max_fd; ++i) {
            if (FD_ISSET(i, &_working_set)) {
                // Controlla se è un socket di ascolto
                bool is_listener = false;
                for (size_t j = 0; j < _instances.size(); ++j) {
                    if (_instances[j]->getSocket() == i) {
                        is_listener = true;
                        _handleNewConnection(i);
                        break;
                    }
                }

                // Se non è un socket di ascolto, è un client
                if (!is_listener) {
                    _handleClientData(i);
                }
            }
        }
    }
}

void Server::_handleNewConnection(int listen_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int new_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (new_fd < 0) {
        std::cerr << "accept() fallita" << std::endl;
        return;
    }

    // Aggiungi il nuovo client al master set
    FD_SET(new_fd, &_master_set);
    if (new_fd > _max_fd)
        _max_fd = new_fd;

    std::cout << "Nuova connessione, socket fd: " << new_fd << std::endl;
}

void Server::_handleClientData(int client_fd) {
    char buffer[4096];
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read <= 0) {
        // Connessione chiusa o errore
        if (bytes_read == 0)
            std::cout << "Socket " << client_fd << " ha chiuso la connessione" << std::endl;
        else
            std::cerr << "recv() fallita su socket " << client_fd << std::endl;
        
        close(client_fd);
        FD_CLR(client_fd, &_master_set);
        return;
    }
    
    // Aggiungi terminatore di stringa e converti in std::string
    buffer[bytes_read] = '\0';
    std::string rawRequest(buffer, bytes_read);
    
    // Stampa la richiesta per debug
    std::cout << "Richiesta ricevuta (fd=" << client_fd << "):" << std::endl;
    std::cout << rawRequest << std::endl;
    
    // Parsa la richiesta
    HttpRequest request;
    std::string errorMsg;
    
    if (!HttpRequest::parse(rawRequest, request, errorMsg)) {
        // Parsing fallito, invia errore 400 Bad Request
        _sendError(client_fd, 400, "Bad Request", errorMsg);
        close(client_fd);
        FD_CLR(client_fd, &_master_set);
        return;
    }
    
    // Handle different HTTP methods
    if (request.getMethod() == "GET") {
        _handleGetRequest(client_fd, request);
    } else if (request.getMethod() == "HEAD") {
        _handleHeadRequest(client_fd, request);
    } else if (request.getMethod() == "POST") {
        _handlePostRequest(client_fd, request);
    } else if (request.getMethod() == "DELETE") {
        _handleDeleteRequest(client_fd, request);
    } else {
        _sendError(client_fd, 405, "Method Not Allowed", "Only GET, HEAD, POST and DELETE methods are currently supported");
    }
    
    close(client_fd);
    FD_CLR(client_fd, &_master_set);
}

const LocationConfig* Server::_findLocationMatch(const std::string& uri, const ServerConfig& server) const {
    std::cout << "Matching location per URI: '" << uri << "'" << std::endl;
    
    // Prima cerca match esatto
    for (size_t i = 0; i < server.locations.size(); ++i) {
        if (uri == server.locations[i].path) {
            std::cout << "Match esatto trovato: " << server.locations[i].path << std::endl;
            return &server.locations[i];
        }
    }
    
    // Poi cerca il prefisso più lungo
    std::string bestMatch = "";
    const LocationConfig* bestLocation = NULL;
    
    for (size_t i = 0; i < server.locations.size(); ++i) {
        std::string locPath = server.locations[i].path;
        
        // Caso speciale per "/"
        if (locPath == "/") {
            if (bestMatch.empty()) {
                bestMatch = "/";
                bestLocation = &server.locations[i];
            }
            continue;
        }
        
        // Verifica se l'URI inizia con il path della location
        if (uri.find(locPath) == 0) {
            // Se la location non termina con /, verifica che l'URI abbia / o sia uguale
            if (locPath[locPath.length()-1] != '/') {
                // Se l'URI è più lungo, deve avere / dopo il prefisso
                if (uri.length() > locPath.length() && uri[locPath.length()] != '/') {
                    continue; // Non è un match valido
                }
            } else {
                // Location termina con /, l'URI deve essere uguale o iniziare con questo prefisso
                // Accettiamo sia /uploads/ che /uploads/file.txt
                if (uri.length() < locPath.length()) {
                    continue; // URI troppo corto per matchare
                }
            }
            
            // Prendiamo il match più lungo
            if (locPath.length() > bestMatch.length()) {
                bestMatch = locPath;
                bestLocation = &server.locations[i];
            }
        }
    }
    
    if (bestLocation) {
        std::cout << "Best location match: " + bestMatch;
    } else {
        std::cout << "Nessuna location trovata per URI: " + uri;
    }
    
    return bestLocation;
}

std::string Server::_getFilePath(const std::string& uri, const LocationConfig* location) {
    if (!location) {
        std::cout << "Nessuna location trovata, usando path predefinito" << std::endl;
        return joinPaths("./", uri.substr(1)); // Rimuovi lo slash iniziale
    }
    
    std::string root = location->root;
    std::string path = uri;
    
    // Ottieni il path relativo rimuovendo il prefisso della location
    if (path.find(location->path) == 0) {
        if (location->path != "/") {
            path = path.substr(location->path.length());
            if (path.empty() || path[0] != '/')
                path = "/" + path;
        }
    }
    
    // Gestione directory e index
    if (path.empty() || path == "/") {
        // Se autoindex è abilitato, non cercare index file
        if (location->autoindex) {
            // Per autoindex, usa il path della location come directory
            if (location->path != "/") {
                path = location->path;
            } else {
                path = "/";
            }
        } else {
            if (!location->index.empty())
                path = "/" + location->index;
            else
                path = "/index.html";
        }
    }
    
    // Rimuovi lo slash iniziale per il join
    if (!path.empty() && path[0] == '/')
        path = path.substr(1);
        
    std::string fullPath = joinPaths(root, path);
    std::cout << "URI: " << uri << ", Root: " << root 
              << ", Path: " << path << ", FullPath: " << fullPath << std::endl;
    
    return fullPath;
}

void Server::_handleGetRequest(int client_fd, const HttpRequest& request) {
    std::cout << "GET " << request.getPath() << std::endl;
    
    // 1. Trova il server config che gestisce questo host
    const ServerConfig* server = NULL;
    std::string host = request.getHeader("host");
    
    // Rimuovi la porta dall'header host, se presente
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos)
        host = host.substr(0, colonPos);
    
    // Debug
    std::cout << "Cercando server per host: '" << host << "'" << std::endl;
    
    // Trova il server config corrispondente
    for (size_t i = 0; i < _servers.size(); ++i) {
        if (_servers[i].server_name == host) {
            server = &_servers[i];
            std::cout << "Trovato server per host: " << host << std::endl;
            break;
        }
    }
    
    // Se non troviamo un server specifico, usa il primo
    if (!server && !_servers.empty()) {
        server = &_servers[0];
        std::cout << "Nessuna corrispondenza, usando server predefinito" << std::endl;
    }
    
    // 2. Trova il location block che combacia con l'URI
    const LocationConfig* location = NULL;
    if (server)
        location = _findLocationMatch(request.getPath(), *server);
    
    // 3. Ottieni il path del file
    std::string filePath = _getFilePath(request.getPath(), location);
    
    // 4. Verifica che il file esista e sia leggibile
    if (!fileExists(filePath)) {
        _sendNotFound(client_fd, request.getPath());
        return;
    }
    
    // 5. Controlla se è una directory
    if (isDirectory(filePath)) {
        // Verifica se autoindex è abilitato
        bool autoindex = location && location->autoindex;
        
        if (autoindex) {
            _sendAutoindex(client_fd, filePath, request.getPath());
        } else {
            // Prova con l'index file
            std::string indexPath = joinPaths(filePath, location && !location->index.empty() ? 
                                               location->index : "index.html");
            
            if (fileExists(indexPath) && isReadable(indexPath)) {
                _sendFile(client_fd, indexPath);
            } else {
                _sendForbidden(client_fd, request.getPath());
            }
        }
        return;
    }
    
    // 6. Verifica permessi di lettura
    if (!isReadable(filePath)) {
        std::cout << "File non leggibile: " << filePath << std::endl;
        _sendForbidden(client_fd, request.getPath());
        return;
    }
    
    // 7. Invia il file
    _sendFile(client_fd, filePath);
}

void Server::_sendFile(int client_fd, const std::string& path) {
    std::string content;
    
    try {
        content = readFile(path);
    } catch (const std::exception& e) {
        std::cerr << "Errore lettura file " << path << ": " << e.what() << std::endl;
        _sendError(client_fd, 500, "Internal Server Error", "Errore lettura file");
        return;
    }
    
    // Crea la risposta
    HttpResponse response;
    response.setStatusCode(200);
    response.setHeader("Content-Type", HttpResponse::getContentType(path));
    response.setBody(content);
    
    // Invia la risposta
    std::string responseStr = response.toString();
    if (send(client_fd, responseStr.c_str(), responseStr.size(), 0) < 0) {
        std::cerr << "Errore invio risposta al client " << client_fd << std::endl;
    } else {
        std::cout << "Risposta 200 OK, " << content.size() << " bytes" << std::endl;
    }
}

void Server::_sendAutoindex(int client_fd, const std::string& path, const std::string& uri) {
    // Lista i contenuti della directory
    std::vector<std::string> files = listDirectory(path);
    
    // Crea HTML
    std::string body = "<html><head><title>Index of " + uri + 
                      "</title></head><body><h1>Index of " + uri + "</h1><ul>";
    
    // Aggiungi parent directory link se non siamo nella root
    if (uri != "/") {
        body += "<li><a href=\"../\">../</a></li>";
    }
    
    for (size_t i = 0; i < files.size(); ++i) {
        // Determina se è una directory
        bool isDir = isDirectory(joinPaths(path, files[i]));
        std::string name = files[i] + (isDir ? "/" : "");
        body += "<li><a href=\"" + uri + 
                (uri[uri.length()-1] != '/' ? "/" : "") + 
                name + "\">" + name + "</a></li>";
    }
    
    body += "</ul></body></html>";
    
    // Crea la risposta
    HttpResponse response;
    response.setStatusCode(200);
    response.setHeader("Content-Type", "text/html");
    response.setBody(body);
    
    // Invia la risposta
    std::string responseStr = response.toString();
    send(client_fd, responseStr.c_str(), responseStr.size(), 0);
    
    std::cout << "Risposta 200 OK (autoindex)" << std::endl;
}

void Server::_sendNotFound(int client_fd, const std::string& uri) {
    std::string body = "<html><head><title>404 Not Found</title></head>"
                       "<body><h1>404 Not Found</h1><p>The requested URL " + 
                       uri + " was not found on this server.</p></body></html>";
    
    // Crea la risposta
    HttpResponse response;
    response.setStatusCode(404);
    response.setHeader("Content-Type", "text/html");
    response.setBody(body);
    
    // Invia la risposta
    std::string responseStr = response.toString();
    send(client_fd, responseStr.c_str(), responseStr.size(), 0);
    
    std::cout << "Risposta 404 Not Found per " << uri << std::endl;
}

void Server::_sendForbidden(int client_fd, const std::string& uri) {
    std::string body = "<html><head><title>403 Forbidden</title></head>"
                       "<body><h1>403 Forbidden</h1><p>You don't have permission to access " +
                       uri + " on this server.</p></body></html>";
    
    // Crea la risposta
    HttpResponse response;
    response.setStatusCode(403);
    response.setHeader("Content-Type", "text/html");
    response.setBody(body);
    
    // Invia la risposta
    std::string responseStr = response.toString();
    send(client_fd, responseStr.c_str(), responseStr.size(), 0);
    
    std::cout << "Risposta 403 Forbidden per " << uri << std::endl;
}

void Server::_sendError(int client_fd, int statusCode, const std::string& statusText, const std::string& message) {
    // Crea HTML per l'errore
    std::string body = "<html><head><title>" + to_string98(statusCode) + " " + statusText + 
                      "</title></head><body><h1>" + to_string98(statusCode) + " " + 
                      statusText + "</h1><p>" + message + "</p></body></html>";
    
    // Crea la risposta
    HttpResponse response;
    response.setStatusCode(statusCode);
    response.setHeader("Content-Type", "text/html");
    response.setBody(body);
    
    // Invia la risposta
    std::string responseStr = response.toString();
    send(client_fd, responseStr.c_str(), responseStr.size(), 0);
    
    std::cout << "Risposta " << statusCode << " " << statusText << std::endl;
}

// Aggiungi questa funzione di log con diversi livelli
enum LogLevel { DEBUG, INFO, WARNING, ERROR };

void log(LogLevel level, const std::string& message) {
    std::string prefix;
    switch (level) {
        case DEBUG: prefix = "[DEBUG] "; break;
        case INFO: prefix = "[INFO] "; break;
        case WARNING: prefix = "[WARNING] "; break;
        case ERROR: prefix = "[ERROR] "; break;
    }
    
    std::cout << prefix << message << std::endl;
}

void Server::_handlePostRequest(int client_fd, const HttpRequest& request) {
    std::cout << "POST " << request.getPath() << std::endl;
    
    // 1. Trova il server config che gestisce questo host
    const ServerConfig* server = NULL;
    std::string host = request.getHeader("host");
    
    // Rimuovi la porta dall'header host, se presente
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos)
        host = host.substr(0, colonPos);
    
    // Trova il server config corrispondente
    for (size_t i = 0; i < _servers.size(); ++i) {
        if (_servers[i].server_name == host) {
            server = &_servers[i];
            break;
        }
    }
    
    // Se non troviamo un server specifico, usa il primo
    if (!server && !_servers.empty()) {
        server = &_servers[0];
    }
    
    // 2. Trova il location block che combacia con l'URI
    const LocationConfig* location = NULL;
    if (server)
        location = _findLocationMatch(request.getPath(), *server);
    
    // 3. Verifica che POST sia permesso
    if (location && !location->methods.empty()) {
        bool postAllowed = false;
        for (size_t i = 0; i < location->methods.size(); ++i) {
            if (location->methods[i] == "POST") {
                postAllowed = true;
                break;
            }
        }
        if (!postAllowed) {
            _sendError(client_fd, 405, "Method Not Allowed", "POST not allowed for this location");
            return;
        }
    }
    
    // 4. Verifica content length limit
    size_t contentLength = request.getContentLength();
    size_t maxBodySize = server ? server->client_max_body_size : 0;
    if (location && location->max_body_size > 0) {
        maxBodySize = location->max_body_size;
    }
    if (maxBodySize > 0 && contentLength > maxBodySize) {
        _sendError(client_fd, 413, "Payload Too Large", "Request body too large");
        return;
    }
    
    // 5. Processa i dati POST
    _sendPostResponse(client_fd, request);
}

void Server::_sendPostResponse(int client_fd, const HttpRequest& request) {
    // Crea HTML di risposta con i risultati dell'upload
    std::ostringstream responseBody;
    responseBody << "<!DOCTYPE html><html><head><title>POST Result</title></head><body>";
    responseBody << "<h1>POST Request Processed Successfully</h1>";
    
    // Mostra i file uploadati
    const std::map<std::string, std::string>& uploadedFiles = request.getUploadedFiles();
    if (!uploadedFiles.empty()) {
        responseBody << "<h2>Uploaded Files:</h2><ul>";
        for (std::map<std::string, std::string>::const_iterator it = uploadedFiles.begin();
             it != uploadedFiles.end(); ++it) {
            responseBody << "<li><strong>" << it->first << ":</strong> " << it->second << "</li>";
        }
        responseBody << "</ul>";
    }
    
    // Mostra i dati del form
    const std::map<std::string, std::string>& postData = request.getPostData();
    if (!postData.empty()) {
        responseBody << "<h2>Form Data:</h2><ul>";
        for (std::map<std::string, std::string>::const_iterator it = postData.begin();
             it != postData.end(); ++it) {
            responseBody << "<li><strong>" << it->first << ":</strong> " << it->second << "</li>";
        }
        responseBody << "</ul>";
    }
    
    if (uploadedFiles.empty() && postData.empty()) {
        responseBody << "<p>No data received in POST request.</p>";
    }
    
    responseBody << "<p><a href=\"" << request.getPath() << "\">Go back</a></p>";
    responseBody << "</body></html>";
    
    // Crea e invia la risposta
    HttpResponse response;
    response.setStatusCode(200);
    response.setHeader("Content-Type", "text/html");
    response.setBody(responseBody.str());
    
    std::string responseStr = response.toString();
    if (send(client_fd, responseStr.c_str(), responseStr.size(), 0) < 0) {
        std::cerr << "Errore invio risposta POST al client " << client_fd << std::endl;
    } else {
        std::cout << "Risposta POST 200 OK inviata" << std::endl;
    }
}

void Server::_handleDeleteRequest(int client_fd, const HttpRequest& request) {
    std::cout << "DELETE " << request.getPath() << std::endl;
    
    // 0. Security check: prevent null byte injection
    std::string requestPath = request.getPath();
    if (requestPath.find('\0') != std::string::npos) {
        _sendErrorResponse(client_fd, 400, "Bad Request: null byte in path");
        return;
    }
    
    // 1. Find the server config that handles this host
    const ServerConfig* server = NULL;
    std::string host = request.getHeader("host");
    
    // Remove port from host header if present
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos)
        host = host.substr(0, colonPos);
    
    // Find matching server config
    for (size_t i = 0; i < _servers.size(); ++i) {
        if (_servers[i].server_name == host) {
            server = &_servers[i];
            break;
        }
    }
    
    // If no specific server found, use the first one
    if (!server && !_servers.empty()) {
        server = &_servers[0];
    }
    
    // 2. Find location block that matches the URI
    const LocationConfig* location = NULL;
    if (server)
        location = _findLocationMatch(request.getPath(), *server);
    
    // 3. Check if DELETE is allowed
    if (location && !location->methods.empty()) {
        bool deleteAllowed = false;
        for (size_t i = 0; i < location->methods.size(); ++i) {
            if (location->methods[i] == "DELETE") {
                deleteAllowed = true;
                break;
            }
        }
        if (!deleteAllowed) {
            _sendDeleteResponse(client_fd, request, false, "DELETE method not allowed for this location");
            return;
        }
    }
    
    // 4. Resolve the file path
    std::string filePath;
    
    if (location) {
        // Use location root
        filePath = location->root + requestPath;
    } else if (server) {
        // Use server root
        filePath = server->root + requestPath;
    } else {
        // Default root
        filePath = "./www" + requestPath;
    }
    
    // Normalize path and prevent directory traversal
    filePath = normalizePath(filePath);
    
    // 5. Enhanced security check: ensure path is within allowed directory
    std::string allowedRoot = location ? location->root : (server ? server->root : "./www");
    allowedRoot = normalizePath(allowedRoot);
    
    // Check for directory traversal attempts
    if (requestPath.find("..") != std::string::npos || 
        requestPath.find("//") != std::string::npos ||
        requestPath.find("./") != std::string::npos) {
        _sendErrorResponse(client_fd, 403, "Forbidden: directory traversal attempt");
        return;
    }

    if (filePath.find(allowedRoot) != 0) {
        _sendErrorResponse(client_fd, 403, "Forbidden: path outside allowed directory");
        return;
    }
    
    // 6. Check if path exists and what type it is (safer approach)
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0) {
        // File/directory doesn't exist
        _sendDeleteResponse(client_fd, request, false, "File not found");
        return;
    }
    
    // 7. Check if it's a directory - we don't delete directories
    if (S_ISDIR(fileStat.st_mode)) {
        _sendDeleteResponse(client_fd, request, false, "Cannot delete directory");
        return;
    }
    
    // 8. Check file permissions (readable implies we can access it)
    if (!isReadable(filePath)) {
        _sendDeleteResponse(client_fd, request, false, "Access denied: insufficient permissions");
        return;
    }
    
    // 9. Attempt to delete the file
    if (unlink(filePath.c_str()) == 0) {
        _sendDeleteResponse(client_fd, request, true, "File deleted successfully");
        std::cout << "File deleted: " << filePath << std::endl;
    } else {
        _sendDeleteResponse(client_fd, request, false, "Failed to delete file: " + std::string(strerror(errno)));
        std::cerr << "Failed to delete " << filePath << ": " << strerror(errno) << std::endl;
    }
}

void Server::_sendDeleteResponse(int client_fd, const HttpRequest& request, bool success, const std::string& message) {
    HttpResponse response;
    std::ostringstream responseBody;
    
    responseBody << "<!DOCTYPE html><html><head><title>DELETE Result</title>";
    responseBody << "<style>body{font-family:Arial,sans-serif;margin:40px;}";
    responseBody << ".success{color:green;} .error{color:red;}</style></head><body>";
    
    if (success) {
        response.setStatusCode(200);
        responseBody << "<h1 class=\"success\">✅ DELETE Successful</h1>";
        responseBody << "<p class=\"success\">" << message << "</p>";
    } else {
        // Determine appropriate error code based on message content
        if (message.find("not found") != std::string::npos || message.find("Cannot delete directory") != std::string::npos) {
            response.setStatusCode(404);
        } else if (message.find("not allowed") != std::string::npos || message.find("Access denied") != std::string::npos) {
            response.setStatusCode(403);
        } else {
            response.setStatusCode(500);
        }
        
        responseBody << "<h1 class=\"error\">❌ DELETE Failed</h1>";
        responseBody << "<p class=\"error\">" << message << "</p>";
    }
    
    responseBody << "<p><strong>Requested path:</strong> " << request.getPath() << "</p>";
    responseBody << "<p><a href=\"" << request.getPath() << "\">← Go back</a></p>";
    responseBody << "</body></html>";
    
    response.setHeader("Content-Type", "text/html");
    response.setBody(responseBody.str());
    
    std::string responseStr = response.toString();
    if (send(client_fd, responseStr.c_str(), responseStr.size(), 0) < 0) {
        std::cerr << "Error sending DELETE response to client " << client_fd << std::endl;
    } else {
        std::cout << "DELETE response " << response.getStatusCode() << " sent" << std::endl;
    }
}

/* ADD this helper method if it doesn't exist
std::string Server::_normalizePath(const std::string& path) {
    std::string normalized = path;
    
    // Replace multiple slashes with single slash
    size_t pos = 0;
    while ((pos = normalized.find("//", pos)) != std::string::npos) {
        normalized.replace(pos, 2, "/");
    }
    
    // Remove trailing slash (except for root)
    if (normalized.length() > 1 && normalized[normalized.length() - 1] == '/') {
        normalized = normalized.substr(0, normalized.length() - 1);
    }
    
    return normalized;
}*/

void Server::_handleHeadRequest(int client_fd, const HttpRequest& request) {
    std::cout << "HEAD " << request.getPath() << std::endl;
    
    // Il HEAD method è identico al GET, ma senza inviare il body
    // Riutilizziamo la stessa logica del GET per generare gli headers
    
    std::string uri = request.getUri();
    
    // 1. Trova il server config che gestisce questo host
    const ServerConfig* server = NULL;
    std::string host = request.getHeader("host");
    
    // Rimuovi la porta dall'header host, se presente
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos)
        host = host.substr(0, colonPos);
    
    // Trova il server config corrispondente
    for (size_t i = 0; i < _servers.size(); ++i) {
        if (_servers[i].server_name == host) {
            server = &_servers[i];
            break;
        }
    }
    
    // Se non troviamo un server specifico, usa il primo
    if (!server && !_servers.empty()) {
        server = &_servers[0];
    }
    
    // 2. Trova il location block che combacia con l'URI
    const LocationConfig* location = NULL;
    if (server)
        location = _findLocationMatch(request.getPath(), *server);
    
    if (!location) {
        // Nessun location match trovato
        _sendHeadError(client_fd, 404, "Not Found");
        return;
    }
    
    // 3. Costruisci il percorso del file
    std::string filePath = _getFilePath(request.getPath(), location);
    filePath = normalizePath(filePath);
    
    // 4. Controlla se il file esiste
    if (!fileExists(filePath)) {
        // Se è una directory, prova con index
        if (isDirectory(filePath)) {
            std::string indexFile = filePath;
            if (indexFile[indexFile.length() - 1] != '/')
                indexFile += "/";
            indexFile += location->index;
            
            if (fileExists(indexFile)) {
                filePath = indexFile;
            } else if (location->autoindex) {
                // Per HEAD su directory con autoindex, invia headers come se fosse HTML
                _sendHeadResponse(client_fd, 200, "text/html", 0); // 0 = unknown content length for directory listing
                return;
            } else {
                _sendHeadError(client_fd, 403, "Forbidden");
                return;
            }
        } else {
            _sendHeadError(client_fd, 404, "Not Found");
            return;
        }
    }
    
    // 5. Controlla se il file è leggibile
    if (!isReadable(filePath)) {
        _sendHeadError(client_fd, 403, "Forbidden");
        return;
    }
    
    // 6. Determina Content-Type e Content-Length
    std::string contentType = HttpResponse::getContentType(filePath);
    
    // Ottieni la dimensione del file senza leggerlo
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0) {
        _sendHeadError(client_fd, 500, "Internal Server Error");
        return;
    }
    
    size_t contentLength = fileStat.st_size;
    
    // 7. Invia solo gli headers (senza body)
    _sendHeadResponse(client_fd, 200, contentType, contentLength);
}

void Server::_sendHeadResponse(int client_fd, int statusCode, const std::string& contentType, size_t contentLength) {
    // Crea HttpResponse ma senza body
    HttpResponse response;
    response.setStatusCode(statusCode);
    response.setHeader("Content-Type", contentType);
    
    if (contentLength > 0) {
        std::ostringstream oss;
        oss << contentLength;
        response.setHeader("Content-Length", oss.str());
    }
    
    // NON settiamo il body per HEAD!
    // response.setBody(""); // Non chiamare setBody
    
    std::string responseStr = response.toString();
    
    if (send(client_fd, responseStr.c_str(), responseStr.size(), 0) < 0) {
        std::cerr << "Errore invio risposta HEAD al client " << client_fd << std::endl;
    } else {
        std::cout << "Risposta HEAD " << statusCode << " inviata (headers only)" << std::endl;
    }
}

void Server::_sendHeadError(int client_fd, int statusCode, const std::string& statusText) {
    // Per errori HEAD, invia solo gli headers dell'errore
    HttpResponse response;
    response.setStatusCode(statusCode);
    response.setHeader("Content-Type", "text/html");
    
    // Calcola la lunghezza dell'errore HTML che AVREMMO inviato
    std::string errorBody = "<html><body><h1>" + to_string98(statusCode) + " " + statusText + "</h1></body></html>";
    std::ostringstream oss;
    oss << errorBody.length();
    response.setHeader("Content-Length", oss.str());
    
    // NON settiamo il body per HEAD!
    
    std::string responseStr = response.toString();
    
    if (send(client_fd, responseStr.c_str(), responseStr.size(), 0) < 0) {
        std::cerr << "Errore invio risposta HEAD error al client " << client_fd << std::endl;
    } else {
        std::cout << "Risposta HEAD " << statusCode << " " << statusText << " inviata (headers only)" << std::endl;
    }
}

void Server::_sendErrorResponse(int client_fd, int statusCode, const std::string& message) {
    HttpResponse response;
    response.setStatusCode(statusCode);
    response.setHeader("Content-Type", "text/html");
    
    std::string body = "<html><body><h1>" + to_string98(statusCode) + " " + 
                      HttpResponse::getStatusMessage(statusCode) + "</h1><p>" + message + "</p></body></html>";
    
    std::ostringstream oss;
    oss << body.length();
    response.setHeader("Content-Length", oss.str());
    response.setBody(body);
    
    std::string responseStr = response.toString();
    
    if (send(client_fd, responseStr.c_str(), responseStr.size(), 0) < 0) {
        std::cerr << "Errore invio risposta error al client " << client_fd << std::endl;
    } else {
        std::cout << "Risposta " << statusCode << " " << HttpResponse::getStatusMessage(statusCode) << " inviata" << std::endl;
    }
}