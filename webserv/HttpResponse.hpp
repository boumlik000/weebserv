#pragma once

#include <string>
#include <map>
#include <vector>

class HttpResponse {
public:
    // OCF and default constructor
    HttpResponse();
    // ... other OCF members ...
    ~HttpResponse();

    // Setters to build the response
    void setStatusCode(int code, const std::string& message);
    void addHeader(const std::string& key, const std::string& value);
    void setBody(const std::vector<char>& body);
    void setBody(const std::string& body);

    // Converts the response object into a sendable string
    std::string buildResponse() const;

private:
    int                                 _statusCode;
    std::string                         _statusMessage;
    std::map<std::string, std::string>  _headers;
    std::vector<char>                   _body;
};