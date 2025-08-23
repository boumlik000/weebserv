#pragma once

#include <string>
#include <map>
#include <vector>

class HttpRequest {
public:
    // OCF and a default constructor
    HttpRequest();
    // ... other OCF members ...
    ~HttpRequest();

    // The main function to parse the raw request buffer
    bool parse(const std::string& raw_request);

    // Getters to access the parsed information
    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string& getHttpVersion() const;
    const std::map<std::string, std::string>& getHeaders() const;
    const std::string& getHeader(const std::string& key) const;
    const std::vector<char>& getBody() const;

private:
    // Member variables to store the parsed data
    std::string                         _method;
    std::string                         _uri;
    std::string                         _http_version;
    std::map<std::string, std::string>  _headers;
    std::vector<char>                   _body;

    // Private helper functions for parsing
    void _parseRequestLine(const std::string& line);
    void _parseHeader(const std::string& line);
};