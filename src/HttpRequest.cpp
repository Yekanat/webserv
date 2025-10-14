#include "HttpRequest.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <cstdlib>
#include <ctime>

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
        
        // NUOVO: Per richieste POST, parsa i dati del body
        if (request._method == "POST") {
            request._parsePostData();
        }
    } else {
        // Nessun body
        request._isComplete = true;
    }

    return true;
}

const std::map<std::string, std::string>& HttpRequest::getPostData() const {
    return _postData;
}

const std::map<std::string, std::string>& HttpRequest::getUploadedFiles() const {
    return _uploadedFiles;
}

bool HttpRequest::hasBody() const {
    return getContentLength() > 0;
}

size_t HttpRequest::getContentLength() const {
    std::map<std::string, std::string>::const_iterator it = _headers.find("content-length");
    if (it != _headers.end()) {
        return static_cast<size_t>(std::atol(it->second.c_str()));
    }
    return 0;
}

std::string HttpRequest::getContentType() const {
    std::map<std::string, std::string>::const_iterator it = _headers.find("content-type");
    if (it != _headers.end()) {
        return it->second;
    }
    return "";
}

void HttpRequest::_parsePostData() {
    if (_isMultipartFormData()) {
        _parseMultipartFormData();
    } else {
        _parseUrlEncodedData();
    }
}

bool HttpRequest::_isMultipartFormData() const {
    std::string contentType = getContentType();
    return contentType.find("multipart/form-data") != std::string::npos;
}

void HttpRequest::_parseUrlEncodedData() {
    // Parse application/x-www-form-urlencoded data
    // Format: key1=value1&key2=value2
    
    std::string data = _body;
    size_t pos = 0;
    
    while (pos < data.length()) {
        size_t ampPos = data.find('&', pos);
        if (ampPos == std::string::npos) {
            ampPos = data.length();
        }
        
        std::string pair = data.substr(pos, ampPos - pos);
        size_t eqPos = pair.find('=');
        
        if (eqPos != std::string::npos) {
            std::string key = pair.substr(0, eqPos);
            std::string value = pair.substr(eqPos + 1);
            
            // URL decode key and value
            key = _urlDecode(key);
            value = _urlDecode(value);
            
            _postData[key] = value;
        }
        
        pos = ampPos + 1;
    }
}

void HttpRequest::_parseMultipartFormData() {
    std::string contentType = getContentType();
    
    // Extract boundary
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) return;
    
    std::string boundary = "--" + contentType.substr(boundaryPos + 9);
    
    // Split body by boundary
    size_t pos = 0;
    while (pos < _body.length()) {
        size_t boundaryStart = _body.find(boundary, pos);
        if (boundaryStart == std::string::npos) break;
        
        size_t nextBoundaryStart = _body.find(boundary, boundaryStart + boundary.length());
        if (nextBoundaryStart == std::string::npos) {
            nextBoundaryStart = _body.length();
        }
        
        // Extract part between boundaries
        std::string part = _body.substr(boundaryStart + boundary.length(), 
                                     nextBoundaryStart - boundaryStart - boundary.length());
        
        _parseMultipartPart(part);
        
        pos = nextBoundaryStart;
    }
}

void HttpRequest::_parseMultipartPart(const std::string& part) {
    // Skip CRLF after boundary
    size_t pos = 0;
    if (part.length() >= 2 && part.substr(0, 2) == "\r\n") {
        pos = 2;
    }
    
    // Parse headers of this part
    std::map<std::string, std::string> partHeaders;
    while (pos < part.length()) {
        size_t lineEnd = part.find("\r\n", pos);
        if (lineEnd == std::string::npos) break;
        
        std::string line = part.substr(pos, lineEnd - pos);
        if (line.empty()) {
            pos = lineEnd + 2;
            break; // End of part headers
        }
        
        // Parse header line
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string name = _trim(line.substr(0, colonPos));
            std::string value = _trim(line.substr(colonPos + 1));
            partHeaders[name] = value;
        }
        
        pos = lineEnd + 2;
    }
    
    // Extract content
    std::string content = part.substr(pos);
    // Remove trailing CRLF
    if (content.length() >= 2 && content.substr(content.length() - 2) == "\r\n") {
        content = content.substr(0, content.length() - 2);
    }
    
    // Parse Content-Disposition header
    std::map<std::string, std::string>::iterator it = partHeaders.find("Content-Disposition");
    if (it != partHeaders.end()) {
        std::string disposition = it->second;
        
        // Extract field name
        size_t namePos = disposition.find("name=\"");
        if (namePos != std::string::npos) {
            namePos += 6; // Skip 'name="'
            size_t nameEnd = disposition.find("\"", namePos);
            if (nameEnd != std::string::npos) {
                std::string fieldName = disposition.substr(namePos, nameEnd - namePos);
                
                // Check if it's a file upload
                size_t filenamePos = disposition.find("filename=\"");
                if (filenamePos != std::string::npos) {
                    // It's a file upload
                    filenamePos += 10; // Skip 'filename="'
                    size_t filenameEnd = disposition.find("\"", filenamePos);
                    if (filenameEnd != std::string::npos) {
                        std::string filename = disposition.substr(filenamePos, filenameEnd - filenamePos);
                        
                        if (!filename.empty()) {
                            // Save file to upload directory
                            std::string uploadPath = _saveUploadedFile(filename, content);
                            _uploadedFiles[fieldName] = uploadPath;
                        }
                    }
                } else {
                    // It's a regular form field
                    _postData[fieldName] = content;
                }
            }
        }
    }
}

std::string HttpRequest::_saveUploadedFile(const std::string& filename, const std::string& content) {
    // Create uploads directory if it doesn't exist
    std::string uploadDir = "./uploads/";
    system(("mkdir -p " + uploadDir).c_str());
    
    // Generate unique filename to avoid conflicts
    std::string uniqueFilename = _generateUniqueFilename(filename);
    std::string fullPath = uploadDir + uniqueFilename;
    
    // Write file
    std::ofstream file(fullPath.c_str(), std::ios::binary);
    if (file.is_open()) {
        file.write(content.c_str(), content.length());
        file.close();
        return fullPath;
    }
    
    return "";
}

std::string HttpRequest::_generateUniqueFilename(const std::string& filename) {
    // Simple implementation: add timestamp
    time_t now = time(0);
    std::ostringstream oss;
    oss << now << "_" << filename;
    return oss.str();
}

std::string HttpRequest::_urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            std::string hex = str.substr(i + 1, 2);
            char ch = static_cast<char>(strtol(hex.c_str(), NULL, 16));
            result += ch;
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}