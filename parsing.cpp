#include "parsing.hpp"

ConfigFile::ConfigFile() : max_size(1024), client_max_body_size_set(false) {}
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
        std::cout << "  - Code " << it->first << ": " << it->second << std::endl;
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


const std::vector<struct ListenInfo>& ConfigFile::getListenInfos() const { return listen_infos; }
const std::map<int, std::string>& ConfigFile::getErrorPages() const { return error_pages; }
unsigned int ConfigFile::getMaxSize() const { return max_size; }
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
    pars_cfile(ac,av);
    
    return 0;
}
