// داخل Client.cpp
#include "client.hpp"
#include <unistd.h> // For read() or recv()
#include <errno.h>  // For error numbers like EAGAIN

template <typename T>
static std::string SSTR(T o) {
    std::stringstream ss;
    ss << o;
    return ss.str();
}

void Client::readRequest() {
    const int BUFFER_SIZE = 4096; // 4KB
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(_fd, buffer, BUFFER_SIZE, MSG_DONTWAIT);
    std::cout<<"request : "<<buffer<<std::endl;
    if (bytes_read > 0) {
        _requestBuffer.append(buffer, bytes_read);
        // (هنا خاصك دير لوجيك باش تعرف واش الطلب كمل)
        // أبسط طريقة هي تقلب على النهاية ديال الـ headers
        if (_requestBuffer.find("\r\n\r\n") != std::string::npos) {
            _state = REQUEST_RECEIVED;
        }
    } else if (bytes_read == 0) {
        _state = DONE; // إذن، الكليان سالا، خاصو يتمسح
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // إلى كان الخطأ ماشي "جرب مرة أخرى"، إذن هو خطأ حقيقي
            _state = DONE; // كنعتبرو الاتصال مات
        }
        // إلى كان الخطأ EAGAIN، مكنديرو والو، كنتسناو epoll تعلمنا مرة أخرى
    }
}
void Client::sendResponse() {
    if (_state != SENDING_RESPONSE) {
        return; // مكنديرو والو إلى مكناش فالحالة ديال الإرسال
    }
    std::string response_str = _httpResponse.buildResponseString();
    // std::string response_str = 
    //             "HTTP/1.1 200 OK\r\n"
    //             "Content-Type: text/html\r\n"
    //             "Content-Length: 139\r\n"
    //             "Connection: close\r\n"
    //             "\r\n"
    //             "<html><head><title>Test Server</title></head>"
    //             "<body><h1>Multiplexing Test Server</h1>"
    //             "<p>Server is working! Request received and processed.</p></body></html>";
    ssize_t bytes_sent = send(_fd, response_str.c_str() + _bytesSent, response_str.length() - _bytesSent, 0);
    if (bytes_sent > 0) {
        std::cout<<"response : "<<response_str<<std::endl;
        _bytesSent += bytes_sent; // كنزيدو داكشي لي تصيفط على المجموع
    } else if (bytes_sent < 0) {
        std::cerr << "Send failed on fd " << _fd 
              << ". Reason: " << strerror(errno) // كيعطيك رسالة الخطأ
              << " (errno: " << errno << ")" << std::endl;
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cout<<"kharya taria "<<std::endl;
            _state = DONE; // خطأ حقيقي، كنقطعو الاتصال
        }
        // إلى كان EAGAIN، يعني buffer ديال الشبكة عامر، غنعاودو نصيفطو من بعد
    }
    // كنتأكدو واش سالينا الإرسال
    if (_bytesSent >= response_str.length()) {
        _state = DONE; // صيفطنا كلشي، إذن سالينا
    }
}
// --- 1. Default Constructor ---
// كنحتاجوه باش نقدر نديرو std::map<int, Client>
Client::Client() : _fd(-1), _state(AWAITING_REQUEST), _config(g_default_config) {
    // كنعطيو للـ fd قيمة غالطة (-1) باش نعرفو أنه مزال ما تخدم
}

// --- Constructor لي كنخدمو بيه بصح ---
Client::Client(int client_fd, const ConfigFile& conf) : _fd(client_fd), _config(conf),_state(AWAITING_REQUEST) {
    std::cout << "HMMMMMMMMMMMMMMMm" << std::endl;
    _config.findLocationFor("/");
    // فاش كنصاوبو client جديد، كنعطيوه الـ fd ديالو مباشرة
    // والحالة الأولية كتكون هي كنتسناو الطلب
}

// --- 2. Copy Constructor ---
Client::Client(const Client& src) : _config(src._config) {
    *this = src;
}

// --- 3. Assignment Operator ---
Client& Client::operator=(const Client& rhs) {
    if (this != &rhs) {
        this->_fd = rhs._fd;
        this->_state = rhs._state;
        this->_requestBuffer = rhs._requestBuffer;
        this->_httpRequest = rhs._httpRequest;
        this->_httpResponse = rhs._httpResponse;
        this->_bytesSent = rhs._bytesSent;
        this->_lastActivity = rhs._lastActivity;
    }
    return *this;
}

// --- 4. Destructor ---
Client::~Client() {
    // فالتصميم لي درنا، الـ Server هو لي مكلف بـ close(this->_fd).
    // هادشي كيخلي الديستراكتور ديال Client بسيط.
    // إلى كان الـ Client هو لي خاصو يسد الـ fd ديالو،
    // كان خاصنا نزيدو هنا: if (this->_fd != -1) close(this->_fd);
    // ولكن من الأحسن تخلي الـ Server هو لي يتحكم فالموارد.
}

// Client.cpp

int Client::getFd() const {
    return this->_fd;
}
ClientState Client::getState() const {
    return this->_state;
}

bool Client::isDone() const {
    return this->_state == DONE;
}




void Client::_buildErrorResponse(int statusCode) {
    
    _httpResponse.setStatusCode(statusCode);
    _httpResponse.setStatusMessage(_config.getErrorPageMessage(statusCode));
    // كنقلبو واش كاين شي صفحة خطأ مخصصة فالكونفيغ

    std::string errorPagePath = _config.getErrorPage(statusCode);
    std::ifstream file(errorPagePath.c_str());
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        _httpResponse.setBody(buffer.str());
        _httpResponse.addHeader("Content-Type", "text/html");
    }
    _httpResponse.addHeader("Content-Length", SSTR(_httpResponse.getBody().size()));
    _state = SENDING_RESPONSE;
}


void Client::_handleDelete(const LocationConfig& location) {
    std::string path = location.root + _httpRequest.getUri();
    // كنتأكدو واش الملف كاين قبل ما نحاولو نمسحوه
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        _buildErrorResponse(404); // 404 Not Found
        return;
    }
    file.close();
    if (std::remove(path.c_str()) != 0) {
        // إلى فشلت عملية المسح (مشكل permissions مثلا)
        _buildErrorResponse(403); // 403 Forbidden
    } else {
        // إلى نجحت عملية المسح
        _httpResponse.setStatusCode(204);
        _httpResponse.setStatusMessage("No Content");
        _state = SENDING_RESPONSE;
    }
}


void Client::_handlePost(const LocationConfig& location) {
    // كنتأكدو أن الكونفيغ فيه مسار محدد للـ upload فهاد الـ location
    if (location.upload.empty()) {
        // إلى ماكانش، يعني هاد الـ location ما مسموحش فيها الـ upload
        _buildErrorResponse(403); // 403 Forbidden
        return;
    }
    // كنصاوبو اسم فريد للملف باش ما يتكتبش شي ملف فوق الآخر
    std::stringstream ss;
    ss << "file_" << time(NULL) << "_" << rand();
    std::string unique_filename = ss.str();
    std::string full_path = location.upload + "/" + unique_filename;
    // كنحلو ملف جديد للكتابة، وبصيغة binary
    std::ofstream file(full_path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        // إلى مقدرناش نصاوبو الملف (مشكل ديال permissions مثلا)
        _buildErrorResponse(500); // 500 Internal Server Error
        return;
    }
    // كنكتبو الـ body ديال الطلب فالملف
    const std::vector<char>& body = _httpRequest.getBody();
    file.write(&body[0], body.size());
    file.close();
    // بناء جواب النجاح
    _httpResponse.setStatusCode(201);
    _httpResponse.setStatusMessage("Created");
    _httpResponse.setBody("<h1>File uploaded successfully!</h1>");
    _httpResponse.addHeader("Content-Type", "text/html");
    _httpResponse.addHeader("Content-Length", SSTR(_httpResponse.getBody().size()));
    
    _state = SENDING_RESPONSE;
}



void Client::_handleGet(const LocationConfig& location) {
    std::string path = location.root + _httpRequest.getUri();
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        _buildErrorResponse(404); // 404 Not Found
        return;
    }
    // كنقراو المحتوى ديال الملف كامل
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string fileContent = buffer.str();
    // بناء الجواب الناجح
    _httpResponse.setStatusCode(200);
    _httpResponse.setStatusMessage("ok");
    _httpResponse.addHeader("Content-Type", "text/html"); // (خاصك دير لوجيك باش تعرف النوع الصحيح)
    _httpResponse.addHeader("Content-Length", SSTR(fileContent.length())); // SSTR كتحول رقم لسترينغ
    _httpResponse.setBody(fileContent);

    _state = SENDING_RESPONSE;
}

void Client::process() {
    // 1. كنتأكدو أننا فالحالة الصحيحة باش نبدأو المعالجة
    if (_state != REQUEST_RECEIVED) {
        return; // إلى مكناش فهاد الحالة، معندنا ما نديرو هنا
    }
    // 2. تحليل (Parsing) الطلب الخام
    if (!_httpRequest.parse(_requestBuffer)) {
        // إلى فشل التحليل، يعني الطلب ماشي بصيغة HTTP صحيحة
        _buildErrorResponse(400); // 400 Bad Request
        return;
    }
    // 3. إيجاد القواعد المناسبة (Routing)
    // _config هو reference للكونفيغ ديال السيرفر كامل
    // خاص تكون عندك دالة ف class ConfigFile كتقلب على الـ location المناسبة
    const LocationConfig& location = _config.findLocationFor(_httpRequest.getUri());
    // 4. التحقق من صحة الطلب بناءً على القواعد
    if (!location.isMethodAllowed(_httpRequest.getMethod())) {
        _buildErrorResponse(405); // 405 Method Not Allowed
        return;
    }
    // 5. تنفيذ الإجراء على حسب الـ Method
    try {
        if (_httpRequest.getMethod() == "GET") {
            _handleGet(location);
        } else if (_httpRequest.getMethod() == "POST") {
            _handlePost(location);
        } else if (_httpRequest.getMethod() == "DELETE") {
            _handleDelete(location);
        } else {
            _buildErrorResponse(501); // 501 Not Implemented
        }
    } catch (const std::exception& e) {
        // إلى وقع أي خطأ غير متوقع أثناء التنفيذ
        _buildErrorResponse(500); // 500 Internal Server Error
    }
}


