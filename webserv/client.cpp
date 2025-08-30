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
    ssize_t bytes_read = recv(_fd, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT);
    if (bytes_read > 0) {
        buffer[bytes_read] = 0;
        std::string tmp = buffer; 
        size_t len = tmp.find("\r\n\r\n");
        if(len != std::string::npos && _requestBuffer.size() + len + 4 >= MAX_REQUEST) //max of  request  hna khas truturni  bad request
         {
            _buildErrorResponse(400);
            return;
         }
        _requestBuffer.append(buffer, bytes_read);
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
    std::string response_str = _httpResponse.buildResponseString(true);
    ssize_t bytes_sent = send(_fd, response_str.c_str() + _bytesSent, response_str.length() - _bytesSent, 0);
    if (bytes_sent > 0) {
        _bytesSent += bytes_sent; // كنزيدو داكشي لي تصيفط على المجموع
    } else if (bytes_sent < 0) {
        std::cerr << "Send failed on fd " << _fd 
              << ". Reason: " << strerror(errno) // كيعطيك رسالة الخطأ
              << " (errno: " << errno << ")" << std::endl;
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
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
Client::Client() : _fd(-1), _state(AWAITING_REQUEST), _config(g_default_config), _bytesSent(0) {
    // كنعطيو للـ fd قيمة غالطة (-1) باش نعرفو أنه مزال ما تخدم
}

// --- Constructor لي كنخدمو بيه بصح ---
Client::Client(int client_fd, const ConfigFile& conf) : _fd(client_fd), _config(conf),_state(AWAITING_REQUEST), _bytesSent(0) {
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
    // return false;
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
        _buildErrorResponse(404); 
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
    const std::string& uri = _httpRequest.getUri();
    const std::string& base = location.path;

    // تأكّد أن uri كاتبدا بـ base
    if (uri.size() < base.size() || uri.compare(0, base.size(), base) != 0) {
        _buildErrorResponse(404);
        return;
    }
    std::string restPath = uri.substr(base.size()); // بلا length

    std::string path = location.root + "/" + restPath;
    bool isFile = false;
    // const char* path = "your_path_here";

    struct stat info;

    if (stat(path.c_str(), &info) < 0) {
        _buildErrorResponse(errno == EACCES ? 403 : 404);
        return;
    }

    std::string mimetype;
    if (S_ISDIR(info.st_mode)) {
        path += location.index;
    } else if (S_ISREG(info.st_mode)) {
        // std::cout << path << " is a regular file-----------------------------------------------------------------------------------"<<std::endl;;
    } else {
        // std::cout << path << " is neither a regular file nor a directory\n";
    }
    _body.open(path.c_str(), std::ios::binary); 
    if (!_body.is_open()) {
        _buildErrorResponse(404); // 404 Not Found
        return;
    }
    // cheak if  it  in my mime type
    int pos = path.rfind('.');
    if(pos != std::string::npos && pos + 1 < path.size())
    {
        mimetype = path.substr(pos + 1); 
        // std::cout << mimetype << std::endl;
    }
    else
        mimetype = "bin";
    if(!_config._mime_type.count(mimetype))
    {
        _buildErrorResponse(408); 
        return;
    }



    std::map<std::string, std::string>::const_iterator it = _config._mime_type.find(mimetype);

    // كنحسبو الحجم ديال الملف
    _body.seekg(0, std::ios::end);
    long long file_size = _body.tellg();
    _body.seekg(0, std::ios::beg);
    
    // بناء الجواب الناجح
    _httpResponse.setStatusCode(200);
    _httpResponse.setStatusMessage("ok");
    _httpResponse.addHeader("Content-Type", it->second); // (خاصك دير لوجيك باش تعرف النوع الصحيح)
    _httpResponse.addHeader("Content-Length", SSTR(file_size)); // SSTR كتحول رقم لسترينغ
    // _httpResponse.setBody(fileContent);

    std::string headers = _httpResponse.buildResponseString(false);
    
    if (send(_fd, headers.c_str(), headers.length(), 0) < 0) {
        // إلى كان مشكل فالإرسال، كنسدو كلشي
        _body.close();
        _state = DONE;
        std::cout << "send failed : " << std::endl;
        return;
    }
    std::cout << "send succes : "<<headers << std::endl;

    // --- المرحلة الثانية: تغيير الحالة باش نبداو نصيفطو الجسم ---
    _state = SENDING_STATIC_FILE;
}

// ف client.cpp
void Client::_sendNextFileChunk() {
    const size_t BUFFER_SIZE = 100; // 8KB
    char buffer[BUFFER_SIZE];

    // كنقراو جزء من الملف من الستريم لي ديجا مفتوح
    _body.read(buffer, BUFFER_SIZE);
    ssize_t bytes_read = _body.gcount();

    if (bytes_read > 0) {
        // كنصيفطو الجزء لي تقرا مباشرة
        ssize_t bytes_sent = send(_fd, buffer, bytes_read, 0);
        if (bytes_sent < 0) {
            // خطأ فالإرسال
            _body.close();
            _state = DONE;
        }
    } else {
        // إلى bytes_read هو 0 أو أقل، يعني سالينا الملف أو وقع خطأ
        std::cout << "  - send succes : "<< std::endl;
        _body.close();
        _state = DONE;
    }
}

void Client::process() {
    // 1. كنتأكدو أننا فالحالة الصحيحة باش نبدأو المعالجة
    if (_state != REQUEST_RECEIVED) {
        // std::cout<<"3iw"<<std::endl;
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


