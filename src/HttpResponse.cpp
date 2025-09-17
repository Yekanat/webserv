#include "HttpResponse.hpp"
#include "utils.hpp"
#include <sstream>

HttpResponse::HttpResponse() : _statusCode(200) {
    // Imposta header di default
    _headers["Server"] = "webserv/1.0";
    _headers["Connection"] = "close";
}

void HttpResponse::setStatusCode(int code) {
    _statusCode = code;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
    _body = body;
    _headers["Content-Length"] = to_string98(_body.length());
}

std::string HttpResponse::toString() const {
    std::ostringstream oss;
    
    // Status line
    oss << "HTTP/1.1 " << _statusCode << " " << getStatusMessage(_statusCode) << "\r\n";
    
    // Headers
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); 
         it != _headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }
    
    // Empty line separating headers from body
    oss << "\r\n";
    
    // Body
    if (!_body.empty()) {
        oss << _body;
    }
    
    return oss.str();
}

std::string HttpResponse::getStatusMessage(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default: return "Unknown";
    }
}

std::string HttpResponse::getContentType(const std::string& path) {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";
        
    std::string ext = path.substr(dot);
    
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".xml") return "application/xml";
    if (ext == ".txt") return "text/plain";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".zip") return "application/zip";
    
    return "application/octet-stream";
}