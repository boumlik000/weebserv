#include "ConfigFile.hpp"

ConfigFile::ConfigFile() : max_size(1024), client_max_body_size_set(false) {
    _mime_type["html"]  = "text/html";
    _mime_type["htm"]   = "text/html";
    _mime_type["shtml"] = "text/html";
    _mime_type["css"]   = "text/css";
    _mime_type["xml"]   = "text/xml";
    _mime_type["gif"]   = "image/gif";
    _mime_type["jpeg"]  = "image/jpeg";
    _mime_type["jpg"]   = "image/jpeg";
    _mime_type["js"]    = "application/javascript";
    _mime_type["atom"]  = "application/atom+xml";
    _mime_type["rss"]   = "application/rss+xml";
    _mime_type["mml"]   = "text/mathml";
    _mime_type["txt"]   = "text/plain";
    _mime_type["jad"]   = "text/vnd.sun.j2me.app-descriptor";
    _mime_type["wml"]   = "text/vnd.wap.wml";
    _mime_type["htc"]   = "text/x-component";
    _mime_type["avif"]  = "image/avif";
    _mime_type["png"]   = "image/png";
    _mime_type["svg"]   = "image/svg+xml";
    _mime_type["svgz"]  = "image/svg+xml";
    _mime_type["tif"]   = "image/tiff";
    _mime_type["tiff"]  = "image/tiff";
    _mime_type["wbmp"]  = "image/vnd.wap.wbmp";
    _mime_type["webp"]  = "image/webp";
    _mime_type["ico"]   = "image/x-icon";
    _mime_type["jng"]   = "image/x-jng";
    _mime_type["bmp"]   = "image/x-ms-bmp";
    _mime_type["woff"]  = "font/woff";
    _mime_type["woff2"] = "font/woff2";
    _mime_type["jar"]   = "application/java-archive";
    _mime_type["war"]   = "application/java-archive";
    _mime_type["ear"]   = "application/java-archive";
    _mime_type["json"]  = "application/json";
    _mime_type["hqx"]   = "application/mac-binhex40";
    _mime_type["doc"]   = "application/msword";
    _mime_type["pdf"]   = "application/pdf";
    _mime_type["ps"]    = "application/postscript";
    _mime_type["eps"]   = "application/postscript";
    _mime_type["ai"]    = "application/postscript";
    _mime_type["rtf"]   = "application/rtf";
    _mime_type["m3u8"]  = "application/vnd.apple.mpegurl";
    _mime_type["kml"]   = "application/vnd.google-earth.kml+xml";
    _mime_type["kmz"]   = "application/vnd.google-earth.kmz";
    _mime_type["xls"]   = "application/vnd.ms-excel";
    _mime_type["eot"]   = "application/vnd.ms-fontobject";
    _mime_type["ppt"]   = "application/vnd.ms-powerpoint";
    _mime_type["odg"]   = "application/vnd.oasis.opendocument.graphics";
    _mime_type["odp"]   = "application/vnd.oasis.opendocument.presentation";
    _mime_type["ods"]   = "application/vnd.oasis.opendocument.spreadsheet";
    _mime_type["odt"]   = "application/vnd.oasis.opendocument.text";
    _mime_type["pptx"]  = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    _mime_type["xlsx"]  = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    _mime_type["docx"]  = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    _mime_type["wmlc"]  = "application/vnd.wap.wmlc";
    _mime_type["wasm"]  = "application/wasm";
    _mime_type["7z"]    = "application/x-7z-compressed";
    _mime_type["cco"]   = "application/x-cocoa";
    _mime_type["jardiff"] = "application/x-java-archive-diff";
    _mime_type["jnlp"]  = "application/x-java-jnlp-file";
    _mime_type["run"]   = "application/x-makeself";
    _mime_type["pl"]    = "application/x-perl";
    _mime_type["pm"]    = "application/x-perl";
    _mime_type["prc"]   = "application/x-pilot";
    _mime_type["pdb"]   = "application/x-pilot";
    _mime_type["rar"]   = "application/x-rar-compressed";
    _mime_type["rpm"]   = "application/x-redhat-package-manager";
    _mime_type["sea"]   = "application/x-sea";
    _mime_type["swf"]   = "application/x-shockwave-flash";
    _mime_type["sit"]   = "application/x-stuffit";
    _mime_type["tcl"]   = "application/x-tcl";
    _mime_type["tk"]    = "application/x-tcl";
    _mime_type["der"]   = "application/x-x509-ca-cert";
    _mime_type["pem"]   = "application/x-x509-ca-cert";
    _mime_type["crt"]   = "application/x-x509-ca-cert";
    _mime_type["xpi"]   = "application/x-xpinstall";
    _mime_type["xhtml"] = "application/xhtml+xml";
    _mime_type["xspf"]  = "application/xspf+xml";
    _mime_type["zip"]   = "application/zip";
    _mime_type["bin"]   = "application/octet-stream";
    _mime_type["exe"]   = "application/octet-stream";
    _mime_type["dll"]   = "application/octet-stream";
    _mime_type["deb"]   = "application/octet-stream";
    _mime_type["dmg"]   = "application/octet-stream";
    _mime_type["iso"]   = "application/octet-stream";
    _mime_type["img"]   = "application/octet-stream";
    _mime_type["msi"]   = "application/octet-stream";
    _mime_type["msp"]   = "application/octet-stream";
    _mime_type["msm"]   = "application/octet-stream";
    _mime_type["mid"]   = "audio/midi";
    _mime_type["midi"]  = "audio/midi";
    _mime_type["kar"]   = "audio/midi";
    _mime_type["mp3"]   = "audio/mpeg";
    _mime_type["ogg"]   = "audio/ogg";
    _mime_type["m4a"]   = "audio/x-m4a";
    _mime_type["ra"]    = "audio/x-realaudio";
    _mime_type["3gpp"]  = "video/3gpp";
    _mime_type["3gp"]   = "video/3gpp";
    _mime_type["ts"]    = "video/mp2t";
    _mime_type["mp4"]   = "video/mp4";
    _mime_type["mpeg"]  = "video/mpeg";
    _mime_type["mpg"]   = "video/mpeg";
    _mime_type["mov"]   = "video/quicktime";
    _mime_type["webm"]  = "video/webm";
    _mime_type["flv"]   = "video/x-flv";
    _mime_type["m4v"]   = "video/x-m4v";
    _mime_type["mng"]   = "video/x-mng";
    _mime_type["asx"]   = "video/x-ms-asf";
    _mime_type["asf"]   = "video/x-ms-asf";
    _mime_type["wmv"]   = "video/x-ms-wmv";
    _mime_type["avi"]   = "video/x-msvideo";
}

ConfigFile::~ConfigFile() {}

std::string ConfigFile::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> ConfigFile::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void ConfigFile::printParsedConfig() const {
    std::cout << "--- Parsed Configuration ---" << std::endl;
    std::cout << "Listen Addresses:" << std::endl;
    for (size_t i = 0; i < listen_infos.size(); ++i) {
        std::cout << "  - " << listen_infos[i].ip << ":" << listen_infos[i].port << std::endl;
    }
    std::cout << "\nError Pages:" << std::endl;
    for (std::map<int, std::string>::const_iterator it = error_pages.begin(); it != error_pages.end(); ++it) {
        std::cout << "  - Code " << it->first << ": " << it->second << " message: " << getErrorPageMessage(it->first) << std::endl;
    }
    std::cout << "\nClient Max Body Size: " << max_size << std::endl;
    std::cout << "\nLocations:" << std::endl;
    for (size_t i = 0; i < location_configs.size(); ++i) {
        const LocationConfig& loc = location_configs[i];
        std::cout << "  Location Path: " << loc.path << std::endl;
        std::cout << "    Allowed Methods: ";
        if (loc.allowed_methods.empty()) {
            std::cout << "off" << std::endl;
        } else {
            for (size_t j = 0; j < loc.allowed_methods.size(); ++j) {
                std::cout << loc.allowed_methods[j] << (j == loc.allowed_methods.size() - 1 ? "" : ", ");
            }
            std::cout << std::endl;
        }
        std::cout << "    Root: " << (loc.root.empty() ? "not set" : loc.root) << std::endl;
        std::cout << "    Index: " << loc.index << std::endl;
        std::cout << "    Autoindex: " << loc.autoindex << std::endl;
        // CHANGED: Show "off" instead of "not set" for upload path
        std::cout << "    Upload Path: " << (loc.upload) << std::endl;
        std::cout << "    CGI Info: ";
        if (loc.cgi_info.empty()) {
            std::cout << "off" << std::endl;
        } else {
            std::cout << std::endl;
            for (std::map<std::string, std::vector<std::string> >::const_iterator it = loc.cgi_info.begin(); it != loc.cgi_info.end(); ++it) {
                std::cout << "      - Path: " << it->first << ", Extensions: ";
                for (size_t k = 0; k < it->second.size(); ++k) {
                    std::cout << it->second[k] << (k == it->second.size() - 1 ? "" : ", ");
                }
                std::cout << std::endl;
            }
        }
        std::cout << "    Redirect: ";
        if (!loc.has_redirect) {
            std::cout << "off" << std::endl;
        } else {
            std::cout << loc.redirect_code << " -> " << loc.redirect_path << std::endl;
        }
        std::cout << std::endl;
    }
    std::cout << "--------------------------" << std::endl;
}

std::string ConfigFile::getErrorPage(int status_code) const {
    std::map<int, std::string>::const_iterator it = error_pages.find(status_code);
    if (it != error_pages.end()) {
        return it->second;
    }
    // Return empty string if status code not found
    return "";
}

std::string ConfigFile::getErrorPageMessage(int status_code) const {
    switch (status_code) {
        case 400:
            return "Bad Request";
        case 416:
            return "Range Not Satisfiable";
        case 415:
            return "Unsupported Media Type";
        case 413:
            return "Content Too Large";
        case 401:
            return "Unauthorized";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 408:
            return "Request Timeout";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        default:
            return "Unknown Error";
    }
}
const std::vector<struct ListenInfo>& ConfigFile::getListenInfos() const { return listen_infos; }
const std::map<int, std::string>& ConfigFile::getErrorPages() const { return error_pages; }
unsigned long ConfigFile::getMaxSize() const { 
    return static_cast<unsigned long>(max_size) * 1024 * 1024;; 
}
const std::vector<LocationConfig>& ConfigFile::getLocationConfigs() const { return location_configs; }
void ConfigFile::setMaxSize(unsigned int size) { max_size = size; }
void ConfigFile::addErrorPage(int code, const std::string& path) { error_pages[code] = path; }

void ConfigFile::addListenInfo(const std::string& ip, int port) {
    ListenInfo info;
    info.ip = ip;
    info.port = port;
    info.addr.sin_family = AF_INET;
    info.addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &info.addr.sin_addr);
    listen_infos.push_back(info);
}

void ConfigFile::addLocationConfig(const LocationConfig& config) { location_configs.push_back(config); }

int Webserv::pars_cfile(int ac, char** av) {
   if (ac != 2) {
        std::cerr << "Usage: " << av[0] << " <config_file>" << std::endl;
        return 1;
    }
    std::string config_file = av[1];
    if (!config.parseConfigFile(config_file)) {
        std::cerr << "Failed to parse configuration file: " << config_file << std::endl;
        return 1;
    }
    std::cout << "Configuration parsed successfully!" << std::endl;
    config.printParsedConfig();
    return 0;
}

int Webserv::start_event(int ac, char** av) {
    if(pars_cfile(ac,av)){
        throw std::runtime_error("wa93at chi 9wada");
    }
    
    return 0;
}