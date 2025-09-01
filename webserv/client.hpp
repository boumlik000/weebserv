#pragma once

#include "ConfigFile.hpp" // الملف لي فيه الكلاس ديال الكونفيغ ديالك
#include <string>
#include <sys/socket.h>
#include "HttpRequest.hpp"  // كلاس خاصة بمعالجة وتخزين الطلب
#include "HttpResponse.hpp" // كلاس خاصة ببناء الجواب
#include <fstream> // باش نقراو الملفات
#include <sstream> // باش نحولو الأرقام لـ string
#include <sys/stat.h>
#include <climits>

// enum باش نتبعو الحالة ديال الكليان






enum ClientState {
    AWAITING_REQUEST,   // كنتسناو الطلب يوصل
    REQUEST_RECEIVED,   // الطلب وصل كاملًا، خاصو يتعالج
    GENERATING_RESPONSE, // كنصاوبو فالجواب (كنقراو ملف أو كندوزو لـ CGI)
    SENDING_RESPONSE,
    SENDING_STATIC_FILE,   // كنصيفطو فالجواب
    DONE                // سالينا معاه، خاصو يتمسح
};
static ConfigFile g_default_config;

class Client {
public:
    // OCF + Constructor لي غنخدمو بيه
    Client(const Client& src);
    Client& operator=(const Client& rhs);
    Client();
    Client(int client_fd, const ConfigFile& conf);
    ~Client();
    void _resetForNewRequest();
    void manageState();

    // === الدوال الرئيسية لي كيستدعيهم السيرفر ===
    void readRequest();  // كتستعمل recv باش تقرا الطلب
    void process();      // كتعالج الطلب وكتصاوب الجواب
    void sendResponse(); // كتستعمل send باش تصيفط الجواب
    void _sendNextFileChunk();
    void _handleGet(const LocationConfig& location);
    void _buildErrorResponse(int statusCode);
    void _handleDelete(const LocationConfig& location);
    void _handlePost(const LocationConfig& location);
    bool _valid_content(std::string);
    void _buildautoindex(std::string);

    time_t getLastActivity() const; // <== زيد هادي

    // === دوال مساعدة ===
    int getFd() const;
    ClientState getState() const;
    bool isDone() const;

private:
    int                 _fd;
    const ConfigFile&        _config;              // السوكيت ديال هاد الكليان (رقم الغرفة)
    ClientState         _state;           // الحالة الحالية ديالو
    
    std::string         _requestBuffer;   // مخزن مؤقت كنجمعو فيه الطلب لي كيجي مقطع
    std::ifstream       _body;
    HttpRequest         _httpRequest;     // أوبجيكت فيه الطلب مفكك ونقي
    HttpResponse        _httpResponse;    // أوبجيكت فيه الجواب لي غادي نصيفطو

    size_t              _bytesSent;       // شحال صيفطنا من الجواب (باش نعرفو فين وصلنا)
    time_t              _lastActivity;    // آخر وقت كان فيه نشاط (باش نحسبو الـ timeout)

    std::vector<char> _fileChunkBuffer;
    size_t            _chunkBytesSent;

    bool                _isRangeRequest;  // واش الطلب فيه Range header
    long long           _rangeStart;      // منين غيبدا الجزء
    long long           _rangeEnd;        // فين غيسالي الجزء
    long long           _fileSize;        // الحجم الكامل للملف
    long long           _bytesToSend;     // شحال من بايت خاصنا نصيفطو فهاد الجواب
    long long           _totalBytesSent;  // شحال صيفطنا فالمجموع
};