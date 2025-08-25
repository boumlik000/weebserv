#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <netinet/in.h>
#include <arpa/inet.h>

// Forward declarations
class ConfigFile;

// Listen structure for sockaddr_in compatibility
struct ListenInfo {
    std::string ip;
    int port;
    struct sockaddr_in addr;

    ListenInfo() : port(0) {
        memset(&addr, 0, sizeof(addr));
    }
};

// Location structure
struct LocationConfig {
    std::string path;
    std::vector<std::string> allowed_methods;
    std::string root;
    std::string index;
    std::string autoindex; // "on" or "off"
    std::string upload;
    std::map<std::string, std::vector<std::string> > cgi_info; // path -> extensions
    std::string redirect_code;
    std::string redirect_path;
    bool has_redirect;
    // bool isMethodAllowed(std::string &method) const
    // {
    //     for(int i = 0; i < allowed_methods.size(); i++)
    //     {
    //         if(allowed_methods[i] == method)
    //             return true;
    //     }
    //     return false;
    // }
    // int maxMatch(std::string &s1, std::string &s2)
    // {
    //     int count = -1;
    //     while(++count && s1[count] && s2[count] && s1[count] == s2[count]);
    //     return count;
    // }
    // LocationConfig *findLocationFor(std::string &uri)
    // {
    //     int max = 0;
    //     LocationConfig *chosen = NULL;

    //     for(int i = 0; i < location_configs.size(); i++)
    //     {
    //         if(location_configs[i].path == uri)
    //             return &location_configs[i];
    //         int match = maxMatch(location_configs[i].path, uri);
    //         if(match > max)
    //         {
    //             chosen = &location_configs[i];
    //             max = match;
    //         }
    //     }
    // }
    // Constructor to set default values
    LocationConfig() : index("index."), autoindex("off"), upload("pages/upload") , has_redirect(false) {}
};

class ConfigFile 
{
    std::vector<std::map<std::string, std::string> > locations;
    unsigned int max_size;
    std::map<int, std::string> error_pages;
    std::vector<struct ListenInfo> listen_infos;
    std::vector<LocationConfig> location_configs;
    bool client_max_body_size_set; // Track if client-max-body-size was already set

    public:
        ConfigFile();
        ~ConfigFile();

        // Parsing methods
        bool parseConfigFile(const std::string& filename);
        bool validateSyntax(const std::string& line);
        bool parseListen(const std::string& value);
        bool parseErrorPage(const std::string& value);
        bool parseClientMaxBodySize(const std::string& value);
        bool parseLocation(const std::string& line);
        bool validateLocationConfig(const LocationConfig& config);
        bool isValidLocationKey(const std::string& key); // New method for key validation

        // Utility methods
        std::string trim(const std::string& str);
        std::vector<std::string> split(const std::string& str, char delimiter);
        bool isValidErrorCode(int code);
        bool isValidIP(const std::string& ip);
        bool isValidPort(int port);
        bool hasOnlyOneSpace(const std::string& str);
        bool containsOnlyPrintableChars(const std::string& str);
        bool isValidHostname(const std::string& hostname);
        bool isListenDuplicate(const std::string& ip, int port);

        // New function to print the configuration
        void printParsedConfig() const;

        // Getters
        const std::vector<struct ListenInfo>& getListenInfos() const;
        const std::map<int, std::string>& getErrorPages() const;
        // Enhanced error page getters
        // const std::map<int, std::string>& getErrorPages() const; // Keep the original
        std::string getErrorPage(int status_code) const;// Get path by status code
        std::string getErrorPageMessage(int status_code) const;
        unsigned int getMaxSize() const;
        const std::vector<LocationConfig>& getLocationConfigs() const;
        
        // Setters (if needed for testing)
        void setMaxSize(unsigned int size);
        void addErrorPage(int code, const std::string& path);
        void addListenInfo(const std::string& ip, int port);
        void addLocationConfig(const LocationConfig& config);
        int pars_cfile(int ac, char** av);
};

class Webserv {
    ConfigFile config;
    public:
        int start_event(int ac, char** av);
};


#endif // WEBSERV_HPP