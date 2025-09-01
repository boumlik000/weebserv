#include "HttpResponse.hpp"
#include <sstream>
#include <ctime> // باش نجبدو الوقت الحالي

// === Orthodox Canonical Form (OCF) ===

HttpResponse::HttpResponse() : _statusCode(0) {}

HttpResponse::HttpResponse(const HttpResponse& src) {
    *this = src;
}

HttpResponse& HttpResponse::operator=(const HttpResponse& rhs) {
    if (this != &rhs) {
        this->_statusCode = rhs._statusCode;
        this->_statusMessage = rhs._statusMessage;
        this->_headers = rhs._headers;
        this->_body = rhs._body;
    }
    return *this;
}

HttpResponse::~HttpResponse() {}

// === Setters ===

void HttpResponse::setStatusCode(int code) {
    this->_statusCode = code;
}

void HttpResponse::setStatusMessage(const std::string& message) {
    this->_statusMessage = message;
}

void HttpResponse::addHeader(const std::string& key, const std::string& value) {
    this->_headers[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
    this->_body.assign(body.begin(), body.end());
}

void HttpResponse::setBody(const std::vector<char>& body) {
    this->_body = body;
}

// === Getters ===

int HttpResponse::getStatusCode() const {
    return this->_statusCode;
}

const std::string& HttpResponse::getStatusMessage() const {
    return this->_statusMessage;
}

const std::vector<char>& HttpResponse::getBody() const {
    return this->_body;
}

// === الدالة الرئيسية لي كتجمع كلشي ===

// دالة مساعدة باش نحولو رقم لـ string
template <typename T>
static std::string SSTR(T o) {
    std::stringstream ss;
    ss << o;
    return ss.str();
}

// دالة مساعدة باش تجيب التاريخ الحالي بصيغة HTTP
static std::string getCurrentHttpDate() {
    char buf[1000];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
    return std::string(buf);
}

std::string HttpResponse::buildResponseString(bool BODY) const {
    // كنستعملو stringstream باش نجمعو فيها الجواب كامل
    std::stringstream response_stream;

    // 1. بناء السطر الأول (Status Line)
    response_stream << "HTTP/1.0 " << this->_statusCode << " " << this->_statusMessage << "\r\n";

    // 2. بناء الـ Headers
    for (std::map<std::string, std::string>::const_iterator it = this->_headers.begin(); it != this->_headers.end(); ++it) {
        response_stream << it->first << ": " << it->second << "\r\n";
    }

    // كنزيدو headers ضرورية إلى ماكانتش ديجا
    if (this->_headers.find("Date") == this->_headers.end()) {
        response_stream << "Date: " << getCurrentHttpDate() << "\r\n";
    }
    if (this->_headers.find("Server") == this->_headers.end()) {
        response_stream << "Server: Webserv/1.0" << "\r\n";
    }
    // Content-Length مهم بزاف
    if (this->_headers.find("Content-Length") == this->_headers.end()) {
        response_stream << "Content-Length: " << this->_body.size() << "\r\n";
    }
    
    // 3. كنزيدو السطر الخاوي لي كيفصل بين الـ Headers و الـ Body
    response_stream << "\r\n";

    // 4. كنزيدو الـ Body
    // كنكتبو الـ body مباشرة من الـ vector<char>
    if(BODY)
        response_stream.write(&this->_body[0], this->_body.size());

    // كنرجعو كلشي مجموع فـ string واحد
    return response_stream.str();
}