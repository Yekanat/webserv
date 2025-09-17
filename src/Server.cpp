#include "Server.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include "utils.hpp"
#include "HttpResponse.hpp"

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
    
    // Ora indirizziamo la richiesta in base al metodo
    if (request.getMethod() == "GET") {
        _handleGetRequest(client_fd, request);
    } else {
        _sendError(client_fd, 405, "Method Not Allowed", "Only GET method is currently supported");
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
        if (!location->index.empty())
            path = "/" + location->index;
        else
            path = "/index.html";
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