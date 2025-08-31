#pragma once

#include <string>
#include <vector>
#include <map>

class HttpResponse {
public:
    // === Orthodox Canonical Form (OCF) ===
    HttpResponse();
    HttpResponse(const HttpResponse& src);
    HttpResponse& operator=(const HttpResponse& rhs);
    ~HttpResponse();

    // === Setters باش نعمرو الجواب ===
    void setStatusCode(int code);
    void setStatusMessage(const std::string& message);
    void addHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& body);
    void setBody(const std::vector<char>& body);

    // === Getters باش نجبدو المعلومات (مفيدين) ===
    int getStatusCode() const;
    const std::string& getStatusMessage() const;
    const std::vector<char>& getBody() const;

    // === الدالة الرئيسية لي كتجمع كلشي ===
    std::string buildResponseString(bool BODY) const;

private:
    // === المتغيرات الأعضاء ===
    int                                 _statusCode;
    std::string                         _statusMessage;
    std::map<std::string, std::string>  _headers;
    std::vector<char>                   _body;
};