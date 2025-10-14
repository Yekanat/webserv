#ifndef HTTP_RESPONSE_HPP
#define HTTP_RESPONSE_HPP

#include <string>
#include <map>
#include "ConfigParser.hpp"
#include "HttpRequest.hpp"

class HttpResponse {
public:
    HttpResponse();

    void setStatusCode(int code);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    std::string toString() const;
    
    static std::string getStatusMessage(int code);
    static std::string getContentType(const std::string& path);

    int getStatusCode() const { return _statusCode; } // ADD getter for status code

private:
    int _statusCode;
    std::map<std::string, std::string> _headers;
    std::string _body;
};

#endif