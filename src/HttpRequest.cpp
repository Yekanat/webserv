#include "HttpRequest.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

HttpRequest::HttpRequest() 
    : _method(), _uri(), _version(), _headers(), _body(), _isComplete(false) {}

const std::string& HttpRequest::getMethod() const {
    return _method;
}

const std::string& HttpRequest::getUri() const {
    return _uri;
}

const std::string& HttpRequest::getVersion() const {
    return _version;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
    return _headers;
}

std::string HttpRequest::getHeader(const std::string& key) const {
    std::string lowercaseKey = _toLower(key);
    std::map<std::string, std::string>::const_iterator it = _headers.find(lowercaseKey);
    if (it != _headers.end())
        return it->second;
    return "";
}

bool HttpRequest::hasHeader(const std::string& key) const {
    std::string lowercaseKey = _toLower(key);
    return _headers.find(lowercaseKey) != _headers.end();
}

const std::string& HttpRequest::getBody() const {
    return _body;
}

bool HttpRequest::isComplete() const {
    return _isComplete;
}

std::string HttpRequest::getPath() const {
    size_t queryPos = _uri.find('?');
    if (queryPos != std::string::npos)
        return _uri.substr(0, queryPos);
    return _uri;
}

std::string HttpRequest::getQueryString() const {
    size_t queryPos = _uri.find('?');
    if (queryPos != std::string::npos && queryPos < _uri.length() - 1)
        return _uri.substr(queryPos + 1);
    return "";
}

std::map<std::string, std::string> HttpRequest::getQueryParams() const {
    return _parseQueryString(getQueryString());
}

std::map<std::string, std::string> HttpRequest::_parseQueryString(const std::string& query) {
    std::map<std::string, std::string> params;
    std::istringstream stream(query);
    std::string pair;
    
    while (std::getline(stream, pair, '&')) {
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos) {
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            params[key] = value;
        } else if (!pair.empty()) {
            params[pair] = "";
        }
    }
    
    return params;
}

std::string HttpRequest::_toLower(const std::string& s) {
    std::string result = s;
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::tolower(result[i]));
    return result;
}

std::string HttpRequest::_trim(const std::string& s) {
    std::string result = s;
    // Trim beginning
    while (!result.empty() && (result[0] == ' ' || result[0] == '\t' || result[0] == '\r' || result[0] == '\n'))
        result.erase(0, 1);
    // Trim end
    while (!result.empty() && (result[result.size()-1] == ' ' || result[result.size()-1] == '\t' || 
           result[result.size()-1] == '\r' || result[result.size()-1] == '\n'))
        result.erase(result.size()-1, 1);
    return result;
}

bool HttpRequest::parse(const std::string& rawRequest, HttpRequest& request, std::string& errorMsg) {
    // Verifica presenza dei delimitatori di fine header
    size_t headerEnd = rawRequest.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        errorMsg = "Incomplete request: missing end of headers";
        return false;
    }

    // Estrai headers
    std::string headers = rawRequest.substr(0, headerEnd);
    std::istringstream stream(headers);
    std::string line;

    // Parse request line
    if (!std::getline(stream, line)) {
        errorMsg = "Empty request";
        return false;
    }
    
    // Rimuovi \r se presente
    if (!line.empty() && line[line.size() - 1] == '\r')
        line.erase(line.size() - 1);
    
    // Parsing di method, URI e version
    std::istringstream requestLine(line);
    if (!(requestLine >> request._method >> request._uri >> request._version)) {
        errorMsg = "Invalid request line format";
        return false;
    }

    // Validazione richiesta HTTP
    if (request._version.substr(0, 5) != "HTTP/") {
        errorMsg = "Invalid HTTP version";
        return false;
    }

    // Parse headers
    while (std::getline(stream, line)) {
        if (line.empty() || (line.size() == 1 && line[0] == '\r'))
            break;
        
        // Rimuovi \r se presente
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        
        // Cerca il separatore degli header (:)
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            errorMsg = "Invalid header format: " + line;
            return false;
        }
        
        std::string key = _toLower(_trim(line.substr(0, colonPos)));
        std::string value = _trim(line.substr(colonPos + 1));
        
        request._headers[key] = value;
    }

    // Estrai il body se presente
    if (headerEnd + 4 < rawRequest.size()) {
        request._body = rawRequest.substr(headerEnd + 4);
        
        // Verifica content-length
        if (request.hasHeader("content-length")) {
            std::istringstream lengthStream(request.getHeader("content-length"));
            size_t contentLength;
            
            if (!(lengthStream >> contentLength)) {
                errorMsg = "Invalid Content-Length value";
                return false;
            }
            
            // Il body è completo se abbiamo ricevuto tutti i byte
            request._isComplete = (request._body.size() >= contentLength);
        } else {
            // Senza Content-Length, assumiamo che il body sia completo
            request._isComplete = true;
        }
    } else {
        // Nessun body
        request._isComplete = true;
    }

    return true;
}