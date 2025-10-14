#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>

class HttpRequest {
public:
    HttpRequest();
    
    // Getters esistenti
    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string& getVersion() const;
    const std::map<std::string, std::string>& getHeaders() const;
    std::string getHeader(const std::string& key) const;
    bool hasHeader(const std::string& key) const;
    const std::string& getBody() const;
    bool isComplete() const;
    
    // Path e query parsing esistenti
    std::string getPath() const;
    std::string getQueryString() const;
    std::map<std::string, std::string> getQueryParams() const;

    // NUOVI METODI PER POST
    const std::map<std::string, std::string>& getPostData() const;
    const std::map<std::string, std::string>& getUploadedFiles() const;
    bool hasBody() const;
    size_t getContentLength() const;
    std::string getContentType() const;

    // Parser principale esistente
    static bool parse(const std::string& rawRequest, HttpRequest& request, std::string& errorMsg);

private:
    std::string _method;
    std::string _uri;
    std::string _version;
    std::map<std::string, std::string> _headers;
    std::string _body;
    bool _isComplete;
    
    // NUOVI MEMBRI PER POST
    std::map<std::string, std::string> _postData;
    std::map<std::string, std::string> _uploadedFiles;

    // Utility esistenti
    static std::string _toLower(const std::string& s);
    static std::string _trim(const std::string& s);
    static std::map<std::string, std::string> _parseQueryString(const std::string& query);
    
    // NUOVI METODI PRIVATI PER POST
    void _parsePostData();
    void _parseUrlEncodedData();
    void _parseMultipartFormData();
    bool _isMultipartFormData() const;
    void _parseMultipartPart(const std::string& part);
    std::string _saveUploadedFile(const std::string& filename, const std::string& content);
    std::string _generateUniqueFilename(const std::string& filename);
    std::string _urlDecode(const std::string& str);
};

#endif