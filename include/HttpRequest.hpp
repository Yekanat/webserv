#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>

class HttpRequest {
public:
    HttpRequest();
    
    // Getters
    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string& getVersion() const;
    const std::map<std::string, std::string>& getHeaders() const;
    std::string getHeader(const std::string& key) const;
    bool hasHeader(const std::string& key) const;
    const std::string& getBody() const;
    bool isComplete() const;
    
    // Path e query parsing
    std::string getPath() const;
    std::string getQueryString() const;
    std::map<std::string, std::string> getQueryParams() const;

    // Parser principale
    static bool parse(const std::string& rawRequest, HttpRequest& request, std::string& errorMsg);

private:
    std::string _method;
    std::string _uri;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _isComplete;

    // Utility per il parsing
    static std::string _toLower(const std::string& s);
    static std::string _trim(const std::string& s);
    static std::map<std::string, std::string> _parseQueryString(const std::string& query);
};

#endif