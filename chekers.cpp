#include "parsing.hpp"

bool ConfigFile::containsOnlyPrintableChars(const std::string& str) {
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] < 32 || str[i] > 126) {
            if (str[i] != ' ' && str[i] != '\t') {
                return false;
            }
        }
    }
    return true;
}

bool ConfigFile::hasOnlyOneSpace(const std::string& str) {
    bool found_space = false;
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == ' ') {
            if (found_space) {
                return false; // Multiple consecutive spaces
            }
            found_space = true;
        } else if (str[i] == '\t') {
            return false; // Tabs not allowed
        } else {
            found_space = false;
        }
    }
    return true;
}

bool ConfigFile::isValidIP(const std::string& ip) {
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
    return result != 0;
}

bool ConfigFile::isValidHostname(const std::string& hostname) {
    if (hostname == "localhost") {
        return true;
    }
    // Basic validation, can be extended
    return false;
}

bool ConfigFile::isValidPort(int port) {
    return port > 0 && port <= 65535;
}

bool ConfigFile::isValidErrorCode(int code) {
    std::set<int> valid_codes;
    valid_codes.insert(400);
    valid_codes.insert(401);
    valid_codes.insert(403);
    valid_codes.insert(404);
    valid_codes.insert(408);
    valid_codes.insert(500);
    valid_codes.insert(501);
    valid_codes.insert(502);
    valid_codes.insert(503);
    valid_codes.insert(504);
    return valid_codes.find(code) != valid_codes.end();
}

bool ConfigFile::isListenDuplicate(const std::string& ip, int port) {
    for (size_t i = 0; i < listen_infos.size(); ++i) {
        // Check for exact duplicate
        if (listen_infos[i].ip == ip && listen_infos[i].port == port) {
            return true;
        }
        
        // Check for conflicting binds on same port
        if (listen_infos[i].port == port) {
            // If existing bind is 0.0.0.0 and new bind is specific IP, conflict
            if (listen_infos[i].ip == "0.0.0.0" && ip != "0.0.0.0") {
                return true;
            }
            // If new bind is 0.0.0.0 and existing bind is specific IP, conflict
            if (ip == "0.0.0.0" && listen_infos[i].ip != "0.0.0.0") {
                return true;
            }
        }
    }
    return false;
}

// NEW: Method to validate location parameter keys
bool ConfigFile::isValidLocationKey(const std::string& key) {
    std::set<std::string> valid_keys;
    valid_keys.insert("allowed-methods");
    valid_keys.insert("root");
    valid_keys.insert("index");
    valid_keys.insert("autoindex");
    valid_keys.insert("upload");
    valid_keys.insert("cgi-info");
    valid_keys.insert("redirect");
    
    //chnu hia dik return , mafhmthach!XD

    return valid_keys.find(key) != valid_keys.end();
}

bool ConfigFile::validateSyntax(const std::string& line) {
    std::string trimmed = trim(line);
    if (trimmed.empty()) return true;
    if (trimmed[0] == '#') {
        std::cerr << "Error: Comments are not allowed in config file" << std::endl;
        return false;
    }
    if (!containsOnlyPrintableChars(trimmed)) {
        std::cerr << "Error: Line contains unprintable characters: " << line << std::endl;
        return false;
    }
    if (!hasOnlyOneSpace(trimmed)) {
        std::cerr << "Error: Line contains extra spaces or tabs: " << line << std::endl;
        return false;
    }
    return true;
}

bool ConfigFile::parseListen(const std::string& value) 
{
    std::vector<std::string> parts = split(value, ' ');
    if (parts.size() != 2) {
        std::cerr << "Error: Invalid listen format: " << value << std::endl;
        return false;
    }

    std::string ip = parts[0];
    if (parts[1].length() > 5)
    {
        std::cerr << "Error: Invalid port: " << parts[1] << " " << parts[1].length()  << std::endl;
        return false;
    }
    
    int port = atoi(parts[1].c_str());

    if ((!isValidIP(ip) && !isValidHostname(ip)) || !isValidPort(port)) {
        std::cerr << "Error: Invalid IP address or hostname or port : " << ip << ":" << port << std::endl;
        return false;
    }
    if (isListenDuplicate(ip, port)) {
        std::cerr << "Error: Duplicate listen directive: " << ip << ":" << port << std::endl;
        return false;
    }

    ListenInfo info;
    info.ip = ip;
    info.port = port;
    info.addr.sin_family = AF_INET;
    info.addr.sin_port = htons(port);

    if (ip == "localhost") {
        inet_pton(AF_INET, "127.0.0.1", &info.addr.sin_addr);
    } else {
        inet_pton(AF_INET, ip.c_str(), &info.addr.sin_addr);
    }

    listen_infos.push_back(info);
    return true;
}

bool ConfigFile::parseErrorPage(const std::string& value) {
    std::vector<std::string> parts = split(value, ' ');
    if (parts.size() != 2) {
        std::cerr << "Error: Invalid error-page format: " << value << std::endl;
        return false;
    }
    int code = atoi(parts[0].c_str());
    std::string path = parts[1];
    if (!isValidErrorCode(code)) {
        std::cerr << "Error: Invalid error code: " << code << std::endl;
        return false;
    }
    if (path.empty()) {
        std::cerr << "Error: Empty error page path" << std::endl;
        return false;
    }
    
    // Check for duplicate error page definitions
    if (error_pages.find(code) != error_pages.end()) {
        std::cerr << "Error: Duplicate error-page directive for code " << code << std::endl;
        return false;
    }
    
    // Check if the error page file is accessible
    std::ifstream file(path.c_str());
    if (file.good()) {
        // File is accessible, use the custom path
        error_pages[code] = path;
        file.close();
    } else {
        // File is not accessible, use default path
        std::stringstream ss;
        ss << "pages/error/error-" << code << ".html";
        error_pages[code] = ss.str();
        std::cout << "Warning: Error page '" << path << "' not accessible, using default: " << error_pages[code] << std::endl;
    }
    
    return true;
}

bool ConfigFile::parseClientMaxBodySize(const std::string& value) {
    // Check for duplicate client-max-body-size directive
    if (client_max_body_size_set) {
        std::cerr << "Error: Duplicate client-max-body-size directive" << std::endl;
        return false;
    }
    
    for (size_t i = 0; i < value.length(); i++) {
        if (value.length() > 4) {
            std::cerr << "Error: client-max-body-size must be less than 1024" << std::endl;
            return false;
        }
        if (!isdigit(value[i])) {
            std::cerr << "Error: Invalid client-max-body-size format: " << value << std::endl;
            return false;
        }
    }
    max_size = static_cast<unsigned int>(atoi(value.c_str()));
    if (max_size > 1024) {
        std::cerr << "Error: client-max-body-size cannot exceed 1024: " << max_size << std::endl;
        return false;
    }
    
    client_max_body_size_set = true;
    return true;
}

bool ConfigFile::parseLocation(const std::string& line) {
    size_t colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        std::cerr << "Error: Invalid location format: " << line << std::endl;
        return false;
    }

    std::string key = line.substr(0, colon_pos);
    std::string rest = trim(line.substr(colon_pos + 1));

    if (key != "location") {
        return false;
    }

    std::vector<std::string> parts = split(rest, ' ');
    if (parts.empty()) {
        std::cerr << "Error: Location missing path" << std::endl;
        return false;
    }

    LocationConfig config;
    config.path = parts[0];

    for (size_t i = 0; i < location_configs.size(); i++) {
        if (location_configs[i].path == config.path) {
            std::cerr << "Error: Duplicate location path: " << config.path << std::endl;
            return false;
        }
    }

    std::set<std::string> seen_keys;
    std::set<std::string> all_cgi_extensions;
    bool has_allowed_methods = false; // Track if allowed-methods was specified

    for (size_t i = 1; i < parts.size(); i++) {
        std::vector<std::string> param = split(parts[i], '=');
        //ila kant lformat dial the whole key fiha ktr mn 2
        if (param.size() != 2) {
            std::cerr << "Error: Invalid parameter format1111: " << parts[i] << std::endl;
            return false;
        }
        std::string param_key = param[0];
        std::string param_value = param[1];

        // NEW: Validate parameter key format
        if (!isValidLocationKey(param_key)) {
            std::cerr << "Error: Invalid location parameter key: " << param_key << std::endl;
            return false;
        }

        if (param_key != "cgi-info" && seen_keys.find(param_key) != seen_keys.end()) {
            std::cerr << "Error: Duplicate parameter: " << param_key << std::endl;
            return false;
        }
        seen_keys.insert(param_key);

        if (param_key == "allowed-methods") {
            has_allowed_methods = true; // Mark that allowed-methods was specified
            std::vector<std::string> methods = split(param_value, '_');
            for (size_t j = 0; j < methods.size(); j++) {
                if (methods[j].empty()) {
                    std::cerr << "Error: Empty method name in allowed-methods value: " << param_value << std::endl;
                    return false;
                }
                if (methods[j] != "GET" && methods[j] != "POST" && methods[j] != "DELETE") {
                    std::cerr << "Error: Invalid HTTP method: " << methods[j] << std::endl;
                    return false;
                }
                config.allowed_methods.push_back(methods[j]);
            }
        } else if (param_key == "root") {
            config.root = param_value;
        } else if (param_key == "index") {
            config.index = param_value;
        } else if (param_key == "autoindex") {
            if (param_value != "on" && param_value != "off") {
                std::cerr << "Error: Invalid autoindex value: " << param_value << std::endl;
                return false;
            }
            config.autoindex = param_value;
        } else if (param_key == "upload") {
            config.upload = param_value;
        } else if (param_key == "cgi-info") {
            std::vector<std::string> cgi_parts = split(param_value, '_');
            if (cgi_parts.size() < 2) {
                std::cerr << "Error: Invalid cgi-info format: " << param_value << std::endl;
                return false;
            }

            std::string cgi_path = cgi_parts[0];
            if (config.cgi_info.find(cgi_path) != config.cgi_info.end()) {
                std::cerr << "Error: Duplicate cgi-info path: " << cgi_path << std::endl;
                return false;
            }

            std::vector<std::string> extensions;
            for (size_t j = 1; j < cgi_parts.size(); j++) {
                if (cgi_parts[j].empty() || cgi_parts[j][0] != '.') {
                    std::cerr << "Error: CGI extension must start with a dot: " << cgi_parts[j] << std::endl;
                    return false;
                }
                if (!all_cgi_extensions.insert(cgi_parts[j]).second) {
                    std::cerr << "Error: Duplicate CGI extension '" << cgi_parts[j] << "' in location " << config.path << std::endl;
                    return false;
                }
                extensions.push_back(cgi_parts[j]);
            }
            config.cgi_info[cgi_path] = extensions;
        } else if (param_key == "redirect") {
            std::vector<std::string> redirect_parts = split(param_value, '_');
            if (redirect_parts.size() != 2) {
                std::cerr << "Error: Invalid redirect format" << std::endl;
                return false;
            }
            if (redirect_parts[0] != "301" && redirect_parts[0] != "302") {
                std::cerr << "Error: Invalid redirect code: " << redirect_parts[0] << std::endl;
                return false;
            }
            config.redirect_code = redirect_parts[0];
            config.redirect_path = redirect_parts[1];
            config.has_redirect = true;
        }
    }

    // NEW: Set default allowed methods if none were specified
    if (!has_allowed_methods) {
        config.allowed_methods.push_back("GET");
        config.allowed_methods.push_back("POST");
        config.allowed_methods.push_back("DELETE");
    }

    if (!validateLocationConfig(config)) {
        return false;
    }

    location_configs.push_back(config);
    return true;
}

bool ConfigFile::validateLocationConfig(const LocationConfig& config) {
    if (config.has_redirect) {
        return true;
    }
    if (config.root.empty()) {
        std::cerr << "Error: Location '" << config.path << "' must have a 'root' directive if no 'redirect' is specified." << std::endl;
        return false;
    }
    return true;
}

bool ConfigFile::parseConfigFile(const std::string& filename) 
{
    if (filename.length() < 6 || filename.substr(filename.length() - 5) != ".conf") {
        std::cerr << "Error: Configuration file must have a .conf extension." << std::endl;
        return false;
    }

    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open config file: " << filename << std::endl;
        return false;
    }

    std::string line;
    int line_number = 0;
    bool found_root_location = false;

    while (std::getline(file, line)) {
        line_number++;
        std::string trimmed = trim(line);
        // std::cout<<trimmed << "(()()())"<<std::endl;

        //hna ila kant line khawia nskippiwha
        if (trimmed.empty()) continue;

        //hna valdatesyntac kanchikiw biha ila kan chi comment , printable caracters , w spaces
        if (!validateSyntax(line)) {
            std::cerr << "Syntax error at line " << line_number << std::endl;
            return false;
        }

        if (trimmed.find("listen:") == 0) 
        {
            if (!parseListen(trim(trimmed.substr(7)))) return false;
        } else if (trimmed.find("error-page:") == 0) 
        {
            if (!parseErrorPage(trim(trimmed.substr(11)))) return false;
        } else if (trimmed.find("client-max-body-size:") == 0)
        {
            if (!parseClientMaxBodySize(trim(trimmed.substr(21)))) return false;
        } else if (trimmed.find("location:") == 0)
        {
            size_t colon_pos = trimmed.find(':');
            std::string rest = trim(trimmed.substr(colon_pos + 1));
            std::vector<std::string> parts = split(rest, ' ');
            if (!parts.empty() && parts[0] == "/") {
                found_root_location = true;
            }
            if (!parseLocation(trimmed)) return false;
        } else {
            std::cerr << "Error: Unknown directive at line " << line_number << ": " << trimmed << std::endl;
            return false;
        }
    }

    // Set default error pages for any missing error codes
    std::vector<int> default_codes;
    default_codes.push_back(400);
    default_codes.push_back(401);
    default_codes.push_back(403);
    default_codes.push_back(404);
    default_codes.push_back(408);
    default_codes.push_back(500);
    default_codes.push_back(501);
    default_codes.push_back(502);
    default_codes.push_back(503);

    for (size_t i = 0; i < default_codes.size(); ++i) {
        int code = default_codes[i];
        if (error_pages.find(code) == error_pages.end()) {
            std::stringstream ss;
            ss << "pages/error/error-" << code << ".html";
            error_pages[code] = ss.str();
        }
    }

    file.close();
    return true;
}

