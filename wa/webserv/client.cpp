// داخل Client.cpp
#include "client.hpp"
#include <unistd.h> // For read() or recv()
#include <errno.h>  // For error numbers like EAGAIN
#include <dirent.h>     // For opendir(), readdir(), closedir()
#include <sys/stat.h>   // For stat(), S_ISDIR()
#include <ctime>        // For localtime(), strftime()
#include <cstring>      // For strcmp()


template <typename T>
static std::string SSTR(T o) {
    std::stringstream ss;
    ss << o;
    return ss.str();
}


static void parseRangeHeader(const std::string& rangeStr, long long& start, long long& end, long long fileSize) {
    std::string bytesPart = rangeStr.substr(6); // كنحيدو "bytes="
    size_t dashPos = bytesPart.find('-');

    if (dashPos == std::string::npos) { // يلا مكاينش "-"
        start = -1; end = -1; return;
    }

    std::string startStr = bytesPart.substr(0, dashPos);
    std::string endStr = bytesPart.substr(dashPos + 1);

    if (startStr.empty()) { // الحالة ديال "bytes=-500" (آخر 500 بايت)
        start = fileSize - atol(endStr.c_str());
        end = fileSize - 1;
    } else {
        start = atol(startStr.c_str());
        if (endStr.empty()) { // الحالة ديال "bytes=900-" (من 900 للخر)
            long long time = start + (2 * 1024 *1024);
            end = (time > fileSize - 1)? (fileSize - 1) : time;
        } else { // الحالة العادية "bytes=100-200"
            end = atol(endStr.c_str());
        }
    }
}
// فـ client.cpp
void Client::readRequest() {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    bytes_read = recv(_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_read > 0) {
        _lastActivity = time(NULL); // هادي خاص تزيدها فالكود ديال timeout
        _requestBuffer.append(buffer, bytes_read);
    } else if (bytes_read == 0) {
        _state = DONE; // العميل سد الإتصال
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            return ; // خطأ حقيقي فالشبكة
        }
        else
            _state = DONE;
    }
}

void Client::readbody() {
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    bytes_read = recv(_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_read > 0) {
        _requestBuffer.append(buffer, bytes_read);
        _bodySize = _bodySize + bytes_read;
        if(_bodySize > _config.getMaxSize())
        {
            _buildErrorResponse(400);
            return;
        }
        _file.write(_requestBuffer.data(), _requestBuffer.size());
        _requestBuffer.clear();
        _lastActivity = time(NULL);
        if(_bodySize >= _content_len)
        {
                _httpResponse.setStatusCode(201);
                _httpResponse.setStatusMessage("Created");
                std::string body = "<html><body><h1>201 Created</h1><p>Resource has been created successfully.</p></body></html>";
                _httpResponse.setBody(body);
                _httpResponse.addHeader("Content-Type", "text/html");
                _httpResponse.addHeader("Content-Length", SSTR(_httpResponse.getBody().size()));

                _state = SENDING_RESPONSE;
            return;
        }
    }else if (bytes_read == 0) {
        _state = DONE; 
    }else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            return ; 
        }
        else
            _state = DONE;
    }
}

void Client::manageState() {
    bool action_taken;
    do {
        action_taken = false; // كنفترضو أننا مغنديرو والو فهاد الدورة

        // --- المرحلة 1: معالجة الإرسال ---
        if (_state == SENDING_RESPONSE) {
            sendResponse();
            action_taken = true;
        } else if (_state == SENDING_STATIC_FILE) {
            _sendNextFileChunk();
            action_taken = true;
        }
        
        // فاش كنساليو شي مهمة، كنديرو reset وكنكملو الحلقة باش نشوفو واش كاين ما يدار
        if (_state == DONE) {
            _resetForNewRequest();
            // action_taken كتبقى true باش الحلقة تعاود دور وتشوف واش كاين طلب جديد فالبافر
        }

        // --- المرحلة 2: معالجة الطلبات لي فالبافر ---
        if (_state == AWAITING_REQUEST) {
            size_t pos = _requestBuffer.find("\r\n\r\n");
            
            if (pos != std::string::npos) { // لقينا طلب كامل
                action_taken = true; // درنا واحد الإجراء
                std::string raw_request = _requestBuffer.substr(0, pos + 4);
                _requestBuffer.erase(0, pos + 4); // كنحيدو الطلب لي غنعالجو
                _httpRequest.parse(raw_request);
            //     if (!_httpRequest.parse(raw_request)) {
            //         // إلى فشل التحليل، يعني الطلب ماشي بصيغة HTTP صحيحة
            //         _buildErrorResponse(400); // 400 Bad Request
            //         return;
            // } // كنعطيوها الطلب لي استخرجنا
                _state = REQUEST_RECEIVED;
                process(); // دابا process غتخدم على الطلب لي ديجا تـ parse
            }
        }
        if(_state == UPLOAD_FILE)
        {
            readbody();
        }

    } while (action_taken && _state != DONE); // كنبقاو ندورو مادام كنديرو شي حاجة ومزال متصلين
}
void Client::_resetForNewRequest() {
    if (!this->_requestBuffer.empty()) {
        std::cout << "\033[33mWARNING: About to clear a non-empty request buffer for fd "
                  << this->_fd << ". Buffer size: " << this->_requestBuffer.size()
                  << " bytes. Content sample:\n---\n"
                  << this->_requestBuffer.substr(0, 200) // كنطبعو غير 200 حرف لولين
                  << "\n---\033[0m" << std::endl;
    }
    // 1. كنسدو الفيشي لي كنا كنقراو منو (إلى كان مفتوح) باش نحررو الموارد
    if (this->_body.is_open()) {
        this->_body.close();
    }

    // 2. كنخويو البافر لي جمعنا فيه الطلب لقديم
    this->_requestBuffer.clear();

    // 3. كنعاودو نكرييو أوبجيكتات جداد باش نمسحو المعلومات لقديمة ديال الطلب والجواب
    this->_httpRequest = HttpRequest();
    this->_httpResponse = HttpResponse();

    // 4. كنرجعو كاع العدادات ديال الإرسال وديال الـ range للصفر
    this->_bytesSent = 0;
    this->_totalBytesSent = 0;
    this->_bytesToSend = 0;
    this->_isRangeRequest = false;
    this->_rangeStart = 0;
    this->_rangeEnd = 0;
    this->_fileSize = 0;
    
    // 5. وأخيراً، كنرجعو للحالة ديال إنتظار طلب جديد
    // this->_state = AWAITING_REQUEST;

    // (إختياري) زيد هاد السطر باش تشوف فالـ console أن العملية دازت بنجاح
    std::cout << "\033[32mClient " << this->_fd 
              << " is reset. Awaiting new request on the same connection (Keep-Alive).\033[0m" << std::endl;
}
void Client::sendResponse() {
    if (_state != SENDING_RESPONSE) {
        return; // مكنديرو والو إلى مكناش فالحالة ديال الإرسال
    }
    std::string response_str = _httpResponse.buildResponseString(true);
    ssize_t bytes_sent = send(_fd, response_str.c_str() + _bytesSent, response_str.length() - _bytesSent, MSG_NOSIGNAL);
    if (bytes_sent > 0) {
        this->_lastActivity = time(NULL); // <== زيد هاد السطر هنا
        _bytesSent += bytes_sent;
    } else if (bytes_sent < 0) {
        std::cerr << "Send failed on fd " << _fd 
              << ". Reason: " << strerror(errno) // كيعطيك رسالة الخطأ
              << " (errno: " << errno << ")" << std::endl;
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cout << "SEND  LI F sendResponse  <  0 " << std::endl;
            _state = DONE; // خطأ حقيقي، كنقطعو الاتصال
        }
        // إلى كان EAGAIN، يعني buffer ديال الشبكة عامر، غنعاودو نصيفطو من بعد
    }
    // كنتأكدو واش سالينا الإرسال
    if (_bytesSent >= response_str.length()) {
        std::cout << "SEND  LI F sendResponse  SIFTATE KOLCHI " << std::endl;
        _state = DONE; // صيفطنا كلشي، إذن سالينا
    }
}
// --- 1. Default Constructor ---
// كنحتاجوه باش نقدر نديرو std::map<int, Client>
Client::Client() : _fd(-1), _state(AWAITING_REQUEST), _config(g_default_config), _bytesSent(0) , _bodySize(0) , _bodyBufferSize(0) {
    // كنعطيو للـ fd قيمة غالطة (-1) باش نعرفو أنه مزال ما تخدم
}

// --- Constructor لي كنخدمو بيه بصح ---
Client::Client(int client_fd, const ConfigFile& conf) : _fd(client_fd), _config(conf), _state(AWAITING_REQUEST), _bytesSent(0) {
    this->_lastActivity = time(NULL); // <== تأكد أن هاد السطر كاين
}
time_t Client::getLastActivity() const { return this->_lastActivity; }

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

void Client::_buildautoindex(std::string location_path) {
    std::stringstream buffer;
    _httpResponse.setStatusCode(200);
    _httpResponse.setStatusMessage("ok");
    
    // HTML header
    buffer << "<!DOCTYPE html>\n"
           << "<html lang=\"en\">\n"
           << "<head>\n"
           << "    <meta charset=\"UTF-8\">\n"
           << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
           << "    <title>Index of " << location_path << " - The Good Fellas</title>\n"
           << "    <style>\n"
           << "        * {\n"
           << "            margin: 0;\n"
           << "            padding: 0;\n"
           << "            box-sizing: border-box;\n"
           << "        }\n"
           << "        body {\n"
           << "            background: linear-gradient(to bottom, #0b0b2b, #1b2735 70%, #090a0f);\n"
           << "            overflow-x: hidden;\n"
           << "            min-height: 100vh;\n"
           << "            font-family: 'Courier New', monospace;\n"
           << "            color: #ffffff;\n"
           << "            padding: 2rem;\n"
           << "            position: relative;\n"
           << "        }\n"
           << "        .stars {\n"
           << "            width: 1px;\n"
           << "            height: 1px;\n"
           << "            position: fixed;\n"
           << "            top: 0;\n"
           << "            left: 0;\n"
           << "            background: white;\n"
           << "            box-shadow: 2vw 5vh 2px white, 10vw 8vh 2px white, 15vw 15vh 1px white,\n"
           << "                22vw 22vh 1px white, 28vw 12vh 2px white, 32vw 32vh 1px white,\n"
           << "                38vw 18vh 2px white, 42vw 35vh 1px white, 48vw 25vh 2px white,\n"
           << "                53vw 42vh 1px white, 58vw 15vh 2px white, 63vw 38vh 1px white,\n"
           << "                68vw 28vh 2px white, 73vw 45vh 1px white, 78vw 32vh 2px white,\n"
           << "                83vw 48vh 1px white, 88vw 20vh 2px white, 93vw 52vh 1px white,\n"
           << "                98vw 35vh 2px white, 5vw 60vh 1px white, 12vw 65vh 2px white,\n"
           << "                18vw 72vh 1px white, 25vw 78vh 2px white, 30vw 85vh 1px white,\n"
           << "                35vw 68vh 2px white, 40vw 82vh 1px white, 45vw 92vh 2px white,\n"
           << "                50vw 75vh 1px white, 55vw 88vh 2px white, 60vw 95vh 1px white,\n"
           << "                65vw 72vh 2px white, 70vw 85vh 1px white, 75vw 78vh 2px white,\n"
           << "                80vw 92vh 1px white, 85vw 82vh 2px white, 90vw 88vh 1px white,\n"
           << "                95vw 75vh 2px white;\n"
           << "            animation: twinkle 8s infinite linear;\n"
           << "            z-index: -100;\n"
           << "        }\n"
           << "        .shooting-star {\n"
           << "            position: fixed;\n"
           << "            width: 100px;\n"
           << "            height: 2px;\n"
           << "            background: linear-gradient(90deg, white, transparent);\n"
           << "            animation: shoot 3s infinite ease-in;\n"
           << "            z-index: -99;\n"
           << "        }\n"
           << "        .shooting-star:nth-child(2) { top: 20%; left: -100px; animation-delay: 0s; }\n"
           << "        .shooting-star:nth-child(3) { top: 35%; left: -100px; animation-delay: 1s; }\n"
           << "        .shooting-star:nth-child(4) { top: 50%; left: -100px; animation-delay: 2s; }\n"
           << "        .shooting-star:nth-child(5) { top: 65%; left: -100px; animation-delay: 0.5s; }\n"
           << "        .shooting-star:nth-child(6) { top: 80%; left: -100px; animation-delay: 1.5s; }\n"
           << "        @keyframes twinkle {\n"
           << "            0%, 100% { opacity: 0.8; }\n"
           << "            50% { opacity: 0.4; }\n"
           << "        }\n"
           << "        @keyframes shoot {\n"
           << "            0% { transform: translateX(0) translateY(0) rotate(25deg); opacity: 1; }\n"
           << "            100% { transform: translateX(120vw) translateY(50vh) rotate(25deg); opacity: 0; }\n"
           << "        }\n"
           << "        .stars::after {\n"
           << "            content: \"\";\n"
           << "            position: absolute;\n"
           << "            width: 1px;\n"
           << "            height: 1px;\n"
           << "            background: white;\n"
           << "            box-shadow: 8vw 12vh 2px white, 16vw 18vh 1px white, 24vw 25vh 2px white,\n"
           << "                33vw 15vh 1px white, 41vw 28vh 2px white, 49vw 35vh 1px white,\n"
           << "                57vw 22vh 2px white, 65vw 42vh 1px white, 73vw 28vh 2px white,\n"
           << "                81vw 48vh 1px white, 89vw 32vh 2px white, 97vw 45vh 1px white,\n"
           << "                3vw 68vh 2px white, 11vw 75vh 1px white, 19vw 82vh 2px white,\n"
           << "                27vw 88vh 1px white, 35vw 72vh 2px white, 43vw 85vh 1px white,\n"
           << "                51vw 92vh 2px white, 59vw 78vh 1px white;\n"
           << "            animation: twinkle 6s infinite linear reverse;\n"
           << "        }\n"
           << "        .container {\n"
           << "            max-width: 1200px;\n"
           << "            margin: 0 auto;\n"
           << "            position: relative;\n"
           << "            z-index: 10;\n"
           << "        }\n"
           << "        .header {\n"
           << "            text-align: center;\n"
           << "            margin-bottom: 3rem;\n"
           << "        }\n"
           << "        .title {\n"
           << "            font-size: 3rem;\n"
           << "            font-weight: bold;\n"
           << "            color: #008cff;\n"
           << "            text-shadow: 0 0 20px #008cff;\n"
           << "            margin-bottom: 1rem;\n"
           << "        }\n"
           << "        .subtitle {\n"
           << "            font-size: 1.2rem;\n"
           << "            color: #ffffff80;\n"
           << "            margin-bottom: 1rem;\n"
           << "        }\n"
           << "        .path {\n"
           << "            font-size: 1rem;\n"
           << "            color: #008cff;\n"
           << "            text-shadow: 0 0 5px #008cff;\n"
           << "        }\n"
           << "        .index-table {\n"
           << "            width: 100%;\n"
           << "            border-collapse: collapse;\n"
           << "            margin-top: 2rem;\n"
           << "            background: rgba(0, 0, 0, 0.7);\n"
           << "            border-radius: 10px;\n"
           << "            overflow: hidden;\n"
           << "            box-shadow: 0 0 20px rgba(0, 0, 0, 0.5);\n"
           << "            position: relative;\n"
           << "            z-index: 20;\n"
           << "        }\n"
           << "        .index-table th {\n"
           << "            background: rgba(0, 140, 255, 0.2);\n"
           << "            color: #ffffff;\n"
           << "            padding: 1rem;\n"
           << "            text-align: left;\n"
           << "            font-weight: 600;\n"
           << "            text-transform: uppercase;\n"
           << "            letter-spacing: 0.05em;\n"
           << "            border-bottom: 1px solid #ffffff20;\n"
           << "        }\n"
           << "        .index-table td {\n"
           << "            padding: 0.8rem 1rem;\n"
           << "            border-bottom: 1px solid #ffffff10;\n"
           << "            transition: all 0.3s ease;\n"
           << "        }\n"
           << "        .index-table tr:hover {\n"
           << "            background: rgba(0, 140, 255, 0.1);\n"
           << "            transform: translateX(5px);\n"
           << "        }\n"
           << "        .file-link {\n"
           << "            color: #ffffff80;\n"
           << "            text-decoration: none;\n"
           << "            transition: all 0.3s ease;\n"
           << "            display: flex;\n"
           << "            align-items: center;\n"
           << "            gap: 0.5rem;\n"
           << "        }\n"
           << "        .file-link:hover {\n"
           << "            color: #008cff;\n"
           << "            text-shadow: 0 0 5px #008cff;\n"
           << "        }\n"
           << "        .file-icon {\n"
           << "            width: 16px;\n"
           << "            height: 16px;\n"
           << "            display: inline-block;\n"
           << "        }\n"
           << "        .folder-icon { color: #ffd700; }\n"
           << "        .file-icon.html { color: #ff6b35; }\n"
           << "        .file-icon.css { color: #1572b6; }\n"
           << "        .file-icon.js { color: #f7df1e; }\n"
           << "        .file-icon.img { color: #ff69b4; }\n"
           << "        .file-icon.txt { color: #90ee90; }\n"
           << "        .file-icon.json { color: #f7df1e; }\n"
           << "        .file-icon.md { color: #90ee90; }\n"
           << "        .file-size, .file-date {\n"
           << "            color: #ffffff60;\n"
           << "            font-size: 0.9rem;\n"
           << "        }\n"
           << "        .back-button {\n"
           << "            position: fixed;\n"
           << "            top: 2rem;\n"
           << "            left: 2rem;\n"
           << "            padding: 10px 20px;\n"
           << "            text-transform: uppercase;\n"
           << "            border-radius: 8px;\n"
           << "            font-size: 14px;\n"
           << "            font-weight: 500;\n"
           << "            color: #ffffff80;\n"
           << "            background: transparent;\n"
           << "            cursor: pointer;\n"
           << "            border: 1px solid #ffffff80;\n"
           << "            transition: 0.5s ease;\n"
           << "            user-select: none;\n"
           << "            text-decoration: none;\n"
           << "            z-index: 30;\n"
           << "        }\n"
           << "        .back-button:hover {\n"
           << "            color: #ffffff;\n"
           << "            background: #008cff;\n"
           << "            border: 1px solid #008cff;\n"
           << "            text-shadow: 0 0 5px #ffffff, 0 0 10px #ffffff, 0 0 20px #ffffff;\n"
           << "            box-shadow: 0 0 5px #008cff, 0 0 20px #008cff, 0 0 50px #008cff, 0 0 100px #008cff;\n"
           << "        }\n"
           << "        @keyframes sparkleAnim {\n"
           << "            0% { opacity: 1; transform: scale(0); }\n"
           << "            50% { opacity: 1; transform: scale(1); }\n"
           << "            100% { opacity: 0; transform: scale(0); }\n"
           << "        }\n"
           << "    </style>\n"
           << "</head>\n"
           << "<body>\n"
           << "    <div class=\"stars\"></div>\n"
           << "    <div class=\"shooting-star\"></div>\n"
           << "    <div class=\"shooting-star\"></div>\n"
           << "    <div class=\"shooting-star\"></div>\n"
           << "    <div class=\"shooting-star\"></div>\n"
           << "    <div class=\"shooting-star\"></div>\n"
           << "    <a href=\"../\" class=\"back-button\">&larr; Back</a>\n"
           << "    <div class=\"container\">\n"
           << "        <div class=\"header\">\n"
           << "            <div class=\"title\">The Good Fellas</div>\n"
           << "            <div class=\"subtitle\">Directory Index</div>\n"
           << "            <div class=\"path\">Index of " << location_path << "</div>\n"
           << "        </div>\n"
           << "        <table class=\"index-table\">\n"
           << "            <thead>\n"
           << "                <tr>\n"
           << "                    <th>Name</th>\n"
           << "                    <th>Last Modified</th>\n"
           << "                    <th>Size</th>\n"
           << "                    <th>Type</th>\n"
           << "                </tr>\n"
           << "            </thead>\n"
           << "            <tbody>\n";

    // Add parent directory if not root
    if (location_path != "/") {
        buffer << "                <tr>\n"
               << "                    <td>\n"
               << "                        <a href=\"../\" class=\"file-link\">\n"
               << "                            <span class=\"file-icon folder-icon\">📁</span>\n"
               << "                            Parent Directory\n"
               << "                        </a>\n"
               << "                    </td>\n"
               << "                    <td class=\"file-date\">-</td>\n"
               << "                    <td class=\"file-size\">-</td>\n"
               << "                    <td class=\"file-size\">Directory</td>\n"
               << "                </tr>\n";
    }

    
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    
    if ((dir = opendir(location_path.c_str())) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            std::string full_path = location_path;
            if (location_path[location_path.length() - 1] != '/') {
                full_path += "/";
            }
            full_path += entry->d_name;
            
            if (stat(full_path.c_str(), &file_stat) == 0) {
                bool is_directory = S_ISDIR(file_stat.st_mode);
                std::string file_name = entry->d_name;
                std::string file_ext = "";
                
                // Get file extension (C++98 compatible)
                if (!is_directory) {
                    size_t dot_pos = file_name.find_last_of(".");
                    if (dot_pos != std::string::npos) {
                        file_ext = file_name.substr(dot_pos + 1);
                        // Convert to lowercase (C++98 way)
                        for (size_t i = 0; i < file_ext.length(); ++i) {
                            file_ext[i] = tolower(file_ext[i]);
                        }
                    }
                }
                
                // Format file size (C++98 compatible)
                std::string size_str = "-";
                if (!is_directory) {
                    long size = file_stat.st_size;
                    std::stringstream ss;
                    if (size < 1024) {
                        ss << size << " B";
                    } else if (size < 1024 * 1024) {
                        ss << ((size + 512) / 1024) << " KB";
                    } else {
                        ss << ((size + 512 * 1024) / (1024 * 1024)) << " MB";
                    }
                    size_str = ss.str();
                }
                
                // Format date
                char date_str[100];
                struct tm *timeinfo = localtime(&file_stat.st_mtime);
                strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", timeinfo);
                
                // Get icon and type based on file extension
                std::string icon = "📄";
                std::string icon_class = "txt";
                std::string type_str = "File";
                
                if (is_directory) {
                    icon = "📁";
                    icon_class = "folder-icon";
                    type_str = "Directory";
                    file_name += "/";
                } else {
                    if (file_ext == "html" || file_ext == "htm") {
                        icon = "📄"; icon_class = "html"; type_str = "HTML Document";
                    } else if (file_ext == "css") {
                        icon = "📄"; icon_class = "css"; type_str = "CSS Stylesheet";
                    } else if (file_ext == "js") {
                        icon = "📄"; icon_class = "js"; type_str = "JavaScript";
                    } else if (file_ext == "json") {
                        icon = "📄"; icon_class = "json"; type_str = "JSON File";
                    } else if (file_ext == "md") {
                        icon = "📄"; icon_class = "md"; type_str = "Markdown";
                    } else if (file_ext == "png" || file_ext == "jpg" || file_ext == "jpeg" || 
                               file_ext == "gif" || file_ext == "svg" || file_ext == "bmp" || 
                               file_ext == "webp") {
                        icon = "📄"; icon_class = "img"; type_str = "Image";
                    } else if (file_ext == "pdf") {
                        icon = "📄"; icon_class = "txt"; type_str = "PDF Document";
                    } else if (file_ext == "zip" || file_ext == "tar" || file_ext == "gz" || 
                               file_ext == "rar") {
                        icon = "📄"; icon_class = "txt"; type_str = "Archive";
                    } else if (file_ext == "cpp" || file_ext == "c" || file_ext == "h" || 
                               file_ext == "hpp") {
                        icon = "📄"; icon_class = "js"; type_str = "C/C++ Source";
                    } else if (file_ext == "py") {
                        icon = "📄"; icon_class = "js"; type_str = "Python Script";
                    } else if (file_ext == "txt" || file_ext == "log") {
                        icon = "📄"; icon_class = "txt"; type_str = "Text File";
                    }
                }
                
                buffer << "                <tr>\n"
                       << "                    <td>\n"
                       << "                        <a href=\"" << file_name << "\" class=\"file-link\">\n"
                       << "                            <span class=\"file-icon " << icon_class << "\">" << icon << "</span>\n"
                       << "                            " << file_name << "\n"
                       << "                        </a>\n"
                       << "                    </td>\n"
                       << "                    <td class=\"file-date\">" << date_str << "</td>\n"
                       << "                    <td class=\"file-size\">" << size_str << "</td>\n"
                       << "                    <td class=\"file-size\">" << type_str << "</td>\n"
                       << "                </tr>\n";
            }
        }
        closedir(dir);
    }

    buffer << "            </tbody>\n"
           << "        </table>\n"
           << "    </div>\n"
           << "    <script>\n"
           << "        document.addEventListener('DOMContentLoaded', function() {\n"
           << "            var links = document.querySelectorAll('.file-link');\n"
           << "            for (var i = 0; i < links.length; i++) {\n"
           << "                links[i].addEventListener('click', function(e) {\n"
           << "                    var filename = this.textContent.trim();\n"
           << "                    console.log('Accessing: ' + filename);\n"
           << "                    this.style.textShadow = '0 0 20px #008cff';\n"
           << "                    var self = this;\n"
           << "                    setTimeout(function() {\n"
           << "                        self.style.textShadow = '';\n"
           << "                    }, 200);\n"
           << "                });\n"
           << "            }\n"
           << "            document.addEventListener('mousemove', function(e) {\n"
           << "                if (Math.random() > 0.99) {\n"
           << "                    createSparkle(e.clientX, e.clientY);\n"
           << "                }\n"
           << "            });\n"
           << "            function createSparkle(x, y) {\n"
           << "                var sparkle = document.createElement('div');\n"
           << "                sparkle.style.cssText = 'position: fixed; left: ' + x + 'px; top: ' + y + 'px; width: 3px; height: 3px; background: #008cff; border-radius: 50%; pointer-events: none; z-index: 1000; box-shadow: 0 0 10px #008cff; animation: sparkleAnim 1s ease-out forwards;';\n"
           << "                document.body.appendChild(sparkle);\n"
           << "                setTimeout(function() {\n"
           << "                    if (sparkle.parentNode) sparkle.parentNode.removeChild(sparkle);\n"
           << "                }, 1000);\n"
           << "            }\n"
           << "        });\n"
           << "    </script>\n"
           << "</body>\n"
           << "</html>";

    _httpResponse.setBody(buffer.str());
    _httpResponse.addHeader("Content-Type", "text/html");
    _httpResponse.addHeader("Content-Length", SSTR(_httpResponse.getBody().size()));
    _state = SENDING_RESPONSE;
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

    const std::string& uri = _httpRequest.getUri();
    const std::string& base = location.path;

    if (uri.size() < base.size() || uri.compare(0, base.size(), base) != 0) {
        _buildErrorResponse(404);
        return;
    }
    std::string restPath = uri.substr(base.size()); 

    std::string path = location.root + "/" + restPath;
    
    std::ifstream file(path.c_str());
    struct stat info;

    if (stat(path.c_str(), &info) < 0) {
        _buildErrorResponse(errno == EACCES ? 403 : 404);
        return;
    }
    if (S_ISDIR(info.st_mode))
    {
        _buildErrorResponse(405); 
        return;
    }
    if (std::remove(path.c_str()) == 0) {
        // إلى نجحت عملية المسح
        _httpResponse.setStatusCode(204);
        _httpResponse.setStatusMessage("No Content");
        _state = SENDING_RESPONSE;
    }

}


bool Client::_valid_content(std::string content_length)
{
    if (content_length.empty())
        return false;

    char* end_character = NULL;
    errno = 0;
    long number = std::strtol(content_length.c_str(), &end_character, 10);

    if (*end_character != '\0') 
        return false;

    if (errno == ERANGE || number < 0)
        return false;
    if (static_cast<unsigned long>(number) > _config.getMaxSize())
        return false;

    _content_len = number;
    return true;
}


std::string generateRandomName() {
    static bool seeded = false;
    if (!seeded) {
        srand(time(NULL));
        seeded = true;
    }
    
    int randomNum = rand() % 900000 + 100000; // 6-digit number (100000-999999)
    std::ostringstream oss;
    oss << "file_" << randomNum;
    return oss.str();
}
void Client::_handlePost(const LocationConfig& location) {

   if(_state != UPLOAD_FILE)
   {
        const std::string& uri = _httpRequest.getUri();
        const std::string& base = location.path;
        std::string scripttype;

        if (location.upload == "off" && location.cgi_info.empty())
        {
            _buildErrorResponse(405);
            return;
        }

        if (uri.size() < base.size() || uri.compare(0, base.size(), base) != 0) {
            _buildErrorResponse(404);
            return;
        }
        std::string restPath = uri.substr(base.size()); 

        std::string path = location.root + "/" + restPath;
        struct stat info;

        if (stat(path.c_str(), &info) < 0) {
            _buildErrorResponse(errno == EACCES ? 403 : 404);
            return;
        }
        std::cout << "===========================> : " << location.path << std::endl;
        
        //cheack if it  in my cgi  map
        bool is_cgi = false;
        int pos = path.rfind('.');
        scripttype = path.substr(pos + 1);
        if(!location.cgi_info.empty())
        {
            std::map<std::string, std::vector<std::string> >::const_iterator it1 = location.cgi_info.begin();
            while(it1 != location.cgi_info.end())
            {
                std::vector<std::string>::const_iterator index =  it1->second.begin();
                while(index != it1->second.end())
                {
                    if(scripttype == *index)
                    {
                        std::cerr << "=======-=-=-=-=-=" << *index << std::endl;
                        is_cgi = true;
                        break;
                    }
                    index++;
                }
                if(is_cgi)
                    break;
                it1++;
            }
        }
        if(!is_cgi && S_ISREG(info.st_mode)) // if not cgi and  is file
        {
            _buildErrorResponse(405);
            return;
        }


        const std::map<std::string , std::string>&map = _httpRequest.getHeaders();
        std::map<std::string, std::string>::const_iterator it = map.find("Content-Length");
        if(!_httpRequest.getHeaders().count("Content-Length") || !_valid_content(it->second))
        {
            _buildErrorResponse(400);
            return;
        }

        if(is_cgi)
        {
            std::cout << "=====================================this is cgi====================" << std::endl;
            _buildErrorResponse(1000);
            return;
            // cgi handl  and  reciv to tmp;
        }
        else // upload her
        {
            std::cout << "=====================================this is upload====================" << std::endl;
            if (location.upload == "off" || S_ISREG(info.st_mode))
            {
                _buildErrorResponse(405);
                return;
            }
            
            std::string name = generateRandomName();
            std::string uploadPath = location.upload + "/" + name + ".raw";
            std::cout << "=====================================upload file create====================" << uploadPath << std::endl;
                _file.open(uploadPath.c_str());
                _state = UPLOAD_FILE;

            if(!_file.is_open()) {
                _buildErrorResponse(500);
                return;
            }
        }
    }
    
    // // كنصاوبو اسم فريد للملف باش ما يتكتبش شي ملف فوق الآخر
    // std::stringstream ss;
    // ss << "file_" << time(NULL) << "_" << rand();
    // std::string unique_filename = ss.str();
    // std::string full_path = location.upload + "/" + unique_filename;
    // // كنحلو ملف جديد للكتابة، وبصيغة binary
    // std::ofstream file(full_path.c_str(), std::ios::binary);
    // if (!file.is_open()) {
    //     // إلى مقدرناش نصاوبو الملف (مشكل ديال permissions مثلا)
    //     _buildErrorResponse(500); // 500 Internal Server Error
    //     return;
    // }
    // // كنكتبو الـ body ديال الطلب فالملف
    // const std::vector<char>& body = _httpRequest.getBody();
    // file.write(&body[0], body.size());
    // file.close();
    // // بناء جواب النجاح
    // _httpResponse.setStatusCode(201);
    // _httpResponse.setStatusMessage("Created");
    // _httpResponse.setBody("<h1>File uploaded successfully!</h1>");
    // _httpResponse.addHeader("Content-Type", "text/html");
    // _httpResponse.addHeader("Content-Length", SSTR(_httpResponse.getBody().size()));
    
    // _state = SENDING_RESPONSE;
}




void Client::_handleGet(const LocationConfig& location) {
    const std::string& uri = _httpRequest.getUri();
    const std::string& base = location.path;
    std::string mimetype;

    // تأكّد أن uri كاتبدا بـ base
    if (uri.size() < base.size() || uri.compare(0, base.size(), base) != 0) {
        _buildErrorResponse(404);
        return;
    }
    std::string restPath = uri.substr(base.size()); // بلا length

    std::string path = location.root + "/" + restPath;
    

    // const char* path = "your_path_here";

    struct stat info;

    if (stat(path.c_str(), &info) < 0) {
        _buildErrorResponse(errno == EACCES ? 403 : 404);
        return;
    }

    if (S_ISDIR(info.st_mode)) {
        if(location.autoindex == "on" && location.index == "off")
        {

            std::cout  << "    = = == ==   " << path << std::endl;
            std::cout << " __ __ + + + + + + + + == " << location.path << std::endl;
            
            
            _buildautoindex(path);
            return;
        }
        path += location.index;
    } else if (S_ISREG(info.st_mode)) {
        // std::cout << path << " is a regular file-----------------------------------------------------------------------------------"<<std::endl;;
    } else {
        // std::cout << path << " is neither a regular file nor a directory\n";
    }





    _body.open(path.c_str(), std::ios::binary);
    // ==> زيد هاد التيست هنا
    if (!_body.good()) {
        std::cerr << "\033[31mCRITICAL: _body stream is NOT good right after open()!\033[0m" << std::endl;
        std::cerr << "  eofbit: " << _body.eof() << ", failbit: " << _body.fail() 
                << ", badbit: " << _body.bad() << std::endl;
    }
    if (!_body.is_open()) {
        _buildErrorResponse(404);
        return;
    }

    _body.seekg(0, std::ios::end);
    _fileSize = _body.tellg();
    _body.clear();
    _body.seekg(0, std::ios::beg);

    // تهيئة المتغيرات ديال الـ range
    _isRangeRequest = false;
    _rangeStart = 0;
    _rangeEnd = _fileSize - 1;

    std::string rangeHeader = _httpRequest.getHeader("Range");
    // std::string rangeHeader;
    std::cout<<" range is :  "<<rangeHeader<<std::endl;
    if (!rangeHeader.empty() && rangeHeader.rfind("bytes=", 0) == 0) {
        _isRangeRequest = true;
        parseRangeHeader(rangeHeader, _rangeStart, _rangeEnd, _fileSize);
    }
    
    // التحقق من صحة الـ range
    if (_isRangeRequest && (_rangeStart < 0 || _rangeEnd < _rangeStart || _rangeEnd >= _fileSize)) {
        _httpResponse.setStatusCode(416);
        _httpResponse.setStatusMessage("Range Not Satisfiable");
        _httpResponse.addHeader("Content-Range", "bytes */" + SSTR(_fileSize));
        _state = SENDING_RESPONSE;
        _body.close();
        return;
    }

    _bytesToSend = (_rangeEnd - _rangeStart) + 1;
    _totalBytesSent = 0;
    _chunkBytesSent = 0;
    
    // بناء الجواب
    if (_isRangeRequest) {
        _httpResponse.setStatusCode(206);
        _httpResponse.setStatusMessage("Partial Content");
        _httpResponse.addHeader("Content-Range", "bytes " + SSTR(_rangeStart) + "-" + SSTR(_rangeEnd) + "/" + SSTR(_fileSize));
        _httpResponse.addHeader("Connection", "keep-alive");
    } else {
        _httpResponse.setStatusCode(200);
        _httpResponse.setStatusMessage("OK");
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
        _buildErrorResponse(415); 
        return;
    }



    std::map<std::string, std::string>::const_iterator it = _config._mime_type.find(mimetype);
    _httpResponse.addHeader("Content-Type", it->second); // (خاصك دير لوجيك باش تعرف النوع الصحيح)

    _httpResponse.addHeader("Content-Length", SSTR(_bytesToSend)); // الطول ديال الجزء لي غيتصيفط
    _httpResponse.addHeader("Accept-Ranges", "bytes"); // مهمة: كتعلم البراوزر أن السيرفر كيقبل طلبات الـ Range

    _body.seekg(_rangeStart); // ==> كنحركو القارئ للبلاصة المطلوبة

        // فـ client.cpp -> _handleGet()

    // ... (من بعد ما كتحسب _rangeStart)

    // ==> السطر لي خاصو ينقز للمكان الصحيح
    _body.seekg(_rangeStart);

    // ==> زيد هاد جوج سطور هنا باش تشوف واش بصح نقز
    long long current_pos = _body.tellg();
    std::cout << "\033[33mDEBUG: Requested start: " << _rangeStart 
            << ", Actual position after seek: " << current_pos << "\033[0m" << std::endl;

    // ... (كمل باقي الكود ديالك)
    std::string headers = _httpResponse.buildResponseString(false);
    if (send(_fd, headers.c_str(), headers.length(), MSG_NOSIGNAL) < 0) {
        _body.close();
        _state = DONE;
        std::cout << "SEND LI F GET FAILED" << std::endl;

        return;
    }
    
    _state = SENDING_STATIC_FILE;
}

// ==============================================================================
// ===                    START OF MODIFIED FUNCTION                          ===
// ==============================================================================

void Client::_sendNextFileChunk() {
    const size_t BUFFER_SIZE = 8192; // 8KB buffer

    // كنتأكدو واش سالينا الإرسال أو الفيشي تسد
    if (!_body.is_open() || _totalBytesSent >= _bytesToSend) {
        if (_body.is_open())
            _body.close();
        _state = DONE;
        return;
    }

    // الخطوة 1: يلا كان البافر الوسيط ديالنا (_fileChunkBuffer) خاوي (تصيفط كامل)، كنعمروه من الفيشي
    if (_chunkBytesSent >= _fileChunkBuffer.size()) {
        long long remainingBytes = _bytesToSend - _totalBytesSent;
        size_t bytesToRead = static_cast<size_t>(std::min((long long)BUFFER_SIZE, remainingBytes));
        
        if (bytesToRead > 0) {
            _fileChunkBuffer.resize(bytesToRead);
            _body.read(&_fileChunkBuffer[0], bytesToRead);
            ssize_t bytesRead = _body.gcount();

            if (bytesRead <= 0) { // خطأ أو نهاية غير متوقعة للملف
                _body.close();
                _state = DONE;
                return;
            }
            _fileChunkBuffer.resize(bytesRead); // كنعدلو الحجم للحجم الحقيقي لي تقرا
            _chunkBytesSent = 0; // كنبداو من الصفر لهاد الشقفة الجديدة
        }
    }

    // الخطوة 2: كنحاولو نصيفطو داكشي لي بقا فالبافر الوسيط ديالنا
    if (_chunkBytesSent < _fileChunkBuffer.size()) {
        size_t bytesToSendFromChunk = _fileChunkBuffer.size() - _chunkBytesSent;
        
        ssize_t bytesSent = send(_fd, &_fileChunkBuffer[_chunkBytesSent], bytesToSendFromChunk, MSG_NOSIGNAL);

        if (bytesSent > 0) {
            this->_lastActivity = time(NULL); // كنحددو وقت النشاط
            _chunkBytesSent += bytesSent;
            _totalBytesSent += bytesSent;
        } else if (bytesSent < 0) {
            // هادي هي الحالة لي كتوقع فالشبكة البطيئة فاش البافر ديال send كيعمر
            // إلى كان الخطأ هو EAGAIN، ماشي مشكل، غنعاودو المحاولة من بعد
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                // خطأ حقيقي، خاصنا نقطعو الاتصال
                std::cerr << "send() error in _sendNextFileChunk: " << strerror(errno) << std::endl;
                _body.close();
                _state = DONE;
            }
            // إلا كان EAGAIN، مكنديرو والو. البيانات لي متصيفطاتش باقا محفوظة فـ _fileChunkBuffer
        }
    }
    
    // الخطوة 3: كنتأكدو للمرة الأخيرة واش سالينا الإرسال
    if (_totalBytesSent >= _bytesToSend) {
        _body.close();
        _state = DONE;
    }
}

// ==============================================================================
// ===                     END OF MODIFIED FUNCTION                           ===
// ==============================================================================


void Client::process() {
    // 1. كنتأكدو أننا فالحالة الصحيحة باش نبدأو المعالجة
    if (_state != REQUEST_RECEIVED) {
        // std::cout<<"3iw"<<std::endl;
        return; // إلى مكناش فهاد الحالة، معندنا ما نديرو هنا
    }
    // 2. تحليل (Parsing) الطلب الخام
    // if (!_httpRequest.parse(_requestBuffer)) {
    //     // إلى فشل التحليل، يعني الطلب ماشي بصيغة HTTP صحيحة
    //     _buildErrorResponse(400); // 400 Bad Request
    //     return;
    // }
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