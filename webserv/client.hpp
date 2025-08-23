#pragma once

#include <string>
#include <sys/socket.h>
#include "HttpRequest.hpp"  // كلاس خاصة بمعالجة وتخزين الطلب
#include "HttpResponse.hpp" // كلاس خاصة ببناء الجواب

// enum باش نتبعو الحالة ديال الكليان
enum ClientState {
    AWAITING_REQUEST,   // كنتسناو الطلب يوصل
    REQUEST_RECEIVED,   // الطلب وصل كاملًا، خاصو يتعالج
    GENERATING_RESPONSE, // كنصاوبو فالجواب (كنقراو ملف أو كندوزو لـ CGI)
    SENDING_RESPONSE,   // كنصيفطو فالجواب
    DONE                // سالينا معاه، خاصو يتمسح
};

class Client {
public:
    // OCF + Constructor لي غنخدمو بيه
    Client();
    Client(int client_fd);
    Client(const Client& src);
    Client& operator=(const Client& rhs);
    ~Client();

    // === الدوال الرئيسية لي كيستدعيهم السيرفر ===
    void readRequest();  // كتستعمل recv باش تقرا الطلب
    void process();      // كتعالج الطلب وكتصاوب الجواب
    void sendResponse(); // كتستعمل send باش تصيفط الجواب

    // === دوال مساعدة ===
    int getFd() const;
    ClientState getState() const;
    bool isDone() const;

private:
    int                 _fd;              // السوكيت ديال هاد الكليان (رقم الغرفة)
    ClientState         _state;           // الحالة الحالية ديالو
    
    std::string         _requestBuffer;   // مخزن مؤقت كنجمعو فيه الطلب لي كيجي مقطع
    HttpRequest         _httpRequest;     // أوبجيكت فيه الطلب مفكك ونقي
    HttpResponse        _httpResponse;    // أوبجيكت فيه الجواب لي غادي نصيفطو

    size_t              _bytesSent;       // شحال صيفطنا من الجواب (باش نعرفو فين وصلنا)
    time_t              _lastActivity;    // آخر وقت كان فيه نشاط (باش نحسبو الـ timeout)
};