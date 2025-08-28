#pragma once

#include "ConfigFile.hpp" // الملف لي فيه الكلاس ديال الكونفيغ ديالك
#include <string>
#include <sys/socket.h>
#include "HttpRequest.hpp"  // كلاس خاصة بمعالجة وتخزين الطلب
#include "HttpResponse.hpp" // كلاس خاصة ببناء الجواب
#include <fstream> // باش نقراو الملفات
#include <sstream> // باش نحولو الأرقام لـ string
// enum باش نتبعو الحالة ديال الكليان
enum ClientState {
    AWAITING_REQUEST,   // كنتسناو الطلب يوصل
    REQUEST_RECEIVED,   // الطلب وصل كاملًا، خاصو يتعالج
    GENERATING_RESPONSE, // كنصاوبو فالجواب (كنقراو ملف أو كندوزو لـ CGI)
    SENDING_RESPONSE,   // كنصيفطو فالجواب
    DONE                // سالينا معاه، خاصو يتمسح
};
static ConfigFile g_default_config;

class Client {
public:
    // OCF + Constructor لي غنخدمو بيه
    Client();
    Client(int client_fd, const ConfigFile& conf);
    Client(const Client& src);
    Client& operator=(const Client& rhs);
    ~Client();

    // === الدوال الرئيسية لي كيستدعيهم السيرفر ===
    void readRequest();  // كتستعمل recv باش تقرا الطلب
    void process();      // كتعالج الطلب وكتصاوب الجواب
    void sendResponse(); // كتستعمل send باش تصيفط الجواب
    void _handleGet(const LocationConfig& location);
    void _buildErrorResponse(int statusCode);
    void _handleDelete(const LocationConfig& location);
    void _handlePost(const LocationConfig& location);
        

    // === دوال مساعدة ===
    int getFd() const;
    ClientState getState() const;
    bool isDone() const;
    const ConfigFile& getConfig() const { return _config; }
private:
    int                 _fd;
    const ConfigFile&        _config;              // السوكيت ديال هاد الكليان (رقم الغرفة)
    ClientState         _state;           // الحالة الحالية ديالو
    
    std::string         _requestBuffer;   // مخزن مؤقت كنجمعو فيه الطلب لي كيجي مقطع
    HttpRequest         _httpRequest;     // أوبجيكت فيه الطلب مفكك ونقي
    HttpResponse        _httpResponse;    // أوبجيكت فيه الجواب لي غادي نصيفطو

    size_t              _bytesSent;       // شحال صيفطنا من الجواب (باش نعرفو فين وصلنا)
    time_t              _lastActivity;    // آخر وقت كان فيه نشاط (باش نحسبو الـ timeout)
};