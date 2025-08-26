#pragma once

#include <string>
#include <vector>
#include <map>

class HttpRequest {
public:
    // === Orthodox Canonical Form (OCF) ===
    HttpRequest();
    HttpRequest(const HttpRequest& src);
    HttpRequest& operator=(const HttpRequest& rhs);
    ~HttpRequest();

    // === الدالة الرئيسية للتحليل ===
    bool parse(const std::string& raw_request);

    // === Getters باش نجبدو المعلومات ===
    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string& getHttpVersion() const;
    const std::string getHeader(const std::string& key) const;
    const std::map<std::string, std::string>& getHeaders() const;
    const std::vector<char>& getBody() const;

private:
    // === المتغيرات الأعضاء ===
    std::string                         _method;
    std::string                         _uri;
    std::string                         _http_version;
    std::map<std::string, std::string>  _headers;
    std::vector<char>                   _body;

    // === الدوال المساعدة الخاصة ===
    void _parseRequestLine(const std::string& line);
    void _parseHeader(const std::string& line);
};