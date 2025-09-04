#include "HttpRequest.hpp"
#include <sstream>
#include <iostream>

// --- دوال مساعدة لـ trim (كنخليوهم static باش يبقاو فهاذ الملف فقط) ---

// كتحيد الفراغات من اللخر ديال السترينغ
static std::string& rtrim(std::string& s, const char* t = " \t\n\r\f\v") {
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// كتحيد الفراغات من الأول ديال السترينغ
static std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v") {
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// كتستعملهم بجوج باش تحيد الفراغات من الأول واللخر
static std::string& trim(std::string& s, const char* t = " \t\n\r\f\v") {
    return ltrim(rtrim(s, t), t);
}

// === Orthodox Canonical Form (OCF) ===

HttpRequest::HttpRequest() {
   
}

HttpRequest::HttpRequest(const HttpRequest& src) {
    *this = src;
}

HttpRequest& HttpRequest::operator=(const HttpRequest& rhs) {
    if (this != &rhs) {
        this->_method = rhs._method;
        this->_uri = rhs._uri;
        this->_http_version = rhs._http_version;
        this->_headers = rhs._headers;
        this->_body = rhs._body;
    }
    return *this;
}

HttpRequest::~HttpRequest() {}

// === الدالة الرئيسية للتحليل ===

bool HttpRequest::parse(const std::string& raw_request) {
    std::stringstream request_stream(raw_request);
    std::string line;

    // 1. تحليل السطر الأول (Request Line)
    if (std::getline(request_stream, line) && !line.empty()) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        _parseRequestLine(line);
    } else {
        return false; // طلب فارغ
    }

    // 2. تحليل الـ Headers
    while (std::getline(request_stream, line) && !line.empty()) {
        if (!line.empty() && line[line.size() - 1] == '\r') {
            line.erase(line.size() - 1);
        }
        if (line.empty()) {
            break;
        }
        _parseHeader(line);
    }
    // 3. كلشي لي بقا من بعد السطر الخاوي هو الـ Body
    std::string body_str;
    std::getline(request_stream, body_str, '\0'); // قرا كلشي لي بقا
    this->_body.assign(body_str.begin(), body_str.end());

    

    return true;
}


// === Getters ===

const std::string& HttpRequest::getMethod() const {
    return this->_method;
}

const std::string& HttpRequest::getUri() const {
    return this->_uri;
}

const std::string& HttpRequest::getHttpVersion() const {
    return this->_http_version;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
    return this->_headers;
}

const std::string HttpRequest::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = this->_headers.find(key);
    if (it != this->_headers.end()) {
        return it->second;
    }
    return ""; // رجع سترينغ خاوي إلى ملقيناش الـ header
}

const std::vector<char>& HttpRequest::getBody() const {
    return this->_body;
}

// === الدوال المساعدة الخاصة ===

void HttpRequest::_parseRequestLine(const std::string& line) {
    std::stringstream line_stream(line);
    line_stream >> this->_method;
    line_stream >> this->_uri;
    line_stream >> this->_http_version;

}

void HttpRequest::_parseHeader(const std::string& line) {
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos && colon_pos > 0) {
        std::string key = line.substr(0, colon_pos);
        std::string value = line.substr(colon_pos + 1);
        this->_headers[trim(key)] = trim(value);
    }
}