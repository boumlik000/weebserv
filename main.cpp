#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "wb0.hpp"
// // Structure to represent a location block
// struct Location {
//     std::string path;
//     std::map<std::string, std::string> directives;
//     int serverFlag;
//     int locationFlag;
    
//     Location() : serverFlag(-1), locationFlag(-1) {}
// };

// // Structure to represent a server block
// struct Server {
//     std::map<std::string, std::string> directives;
//     std::vector<Location> locations;
//     int serverFlag;
    
//     Server() : serverFlag(-1) {}
// };

// class ConfigParser {
// private:
//     std::vector<Server> servers;
//     std::vector<std::string> tokens;
//     size_t currentToken;
//     int currentServerFlag;
//     int currentLocationFlag;

// public:
//     ConfigParser() : currentToken(0), currentServerFlag(-1), currentLocationFlag(-1) {}
    
//     // Convert string to uppercase
//     std::string toUpper(const std::string& str) {
//         std::string result = str;
//         std::transform(result.begin(), result.end(), result.begin(), ::toupper);
//         return result;
//     }
    
//     // Replace colons with equals signs
//     std::string replaceColonWithEquals(const std::string& str) {
//         std::string result = str;
//         size_t pos = result.find(':');
//         if (pos != std::string::npos) {
//             result[pos] = '=';
//         }
//         return result;
//     }
    
//     // Tokenize the configuration file
//     void tokenize(const std::string& content) {
//         std::istringstream iss(content);
//         std::string token;
        
//         while (iss >> token) {
//             if (!token.empty()) {
//                 tokens.push_back(token);
//             }
//         }
        
//         std::cout << "[PARSER] Tokenized " << tokens.size() << " tokens" << std::endl;
//     }
    
//     // Get next token
//     std::string getNextToken() {
//         if (currentToken < tokens.size()) {
//             return tokens[currentToken++];
//         }
//         return "";
//     }
    
//     // Peek at current token without consuming it
//     std::string peekToken() {
//         if (currentToken < tokens.size()) {
//             return tokens[currentToken];
//         }
//         return "";
//     }
    
//     // Parse a key-value pair
//     bool parseDirective(std::map<std::string, std::string>& directives, int serverFlag, int& contentFlag) {
//         std::string key = getNextToken();
//         if (key.empty()) return false;
        
//         // Convert key to uppercase
//         key = toUpper(key);
        
//         std::string value = getNextToken();
//         if (value.empty()) return false;
        
//         // Handle semicolon at the end of value
//         if (value.back() == ';') {
//             value = value.substr(0, value.length() - 1);
//         }
        
//         // Replace colon with equals if present
//         value = replaceColonWithEquals(value);
        
//         directives[key] = value;
        
//         std::cout << "[PARSER] Server Flag: " << serverFlag 
//                   << ", Content Flag: " << contentFlag 
//                   << " -> " << key << " = " << value << std::endl;
        
//         contentFlag++;
//         return true;
//     }
    
//     // Parse a location block
//     bool parseLocation(Server& server, int& serverContentFlag) {
//         std::string locationKeyword = getNextToken(); // should be "location"
//         if (locationKeyword != "location") {
//             std::cout << "[ERROR] Expected 'location', got: " << locationKeyword << std::endl;
//             return false;
//         }
        
//         std::string path = getNextToken();
//         if (path.empty()) {
//             std::cout << "[ERROR] Missing location path" << std::endl;
//             return false;
//         }
        
//         std::string openBrace = getNextToken();
//         if (openBrace != "{") {
//             std::cout << "[ERROR] Expected '{' after location path, got: " << openBrace << std::endl;
//             return false;
//         }
        
//         Location location;
//         location.path = path;
//         location.serverFlag = currentServerFlag;
//         location.locationFlag = currentLocationFlag++;
        
//         std::cout << "[PARSER] Server Flag: " << currentServerFlag 
//                   << ", Content Flag: " << serverContentFlag 
//                   << ", Location " << location.locationFlag 
//                   << " (" << path << ") starting..." << std::endl;
        
//         serverContentFlag++;
//         int locationContentFlag = 0;
        
//         // Parse location directives
//         while (peekToken() != "}" && !peekToken().empty()) {
//             if (peekToken() == "location") {
//                 std::cout << "[ERROR] Nested locations not supported" << std::endl;
//                 return false;
//             }
            
//             if (!parseDirective(location.directives, location.serverFlag, locationContentFlag)) {
//                 std::cout << "[ERROR] Failed to parse location directive" << std::endl;
//                 return false;
//             }
//         }
        
//         std::string closeBrace = getNextToken();
//         if (closeBrace != "}") {
//             std::cout << "[ERROR] Expected '}' to close location block, got: " << closeBrace << std::endl;
//             return false;
//         }
        
//         server.locations.push_back(location);
//         return true;
//     }
    
//     // Parse a server block
//     bool parseServer() {
//         std::string serverKeyword = getNextToken(); // should be "server"
//         if (serverKeyword != "server") {
//             std::cout << "[ERROR] Expected 'server', got: " << serverKeyword << std::endl;
//             return false;
//         }
        
//         std::string openBrace = getNextToken();
//         if (openBrace != "{") {
//             std::cout << "[ERROR] Expected '{' after server, got: " << openBrace << std::endl;
//             return false;
//         }
        
//         currentServerFlag++;
//         currentLocationFlag = 0;
        
//         Server server;
//         server.serverFlag = currentServerFlag;
        
//         std::cout << "[PARSER] Parsing server block (Flag: " << server.serverFlag << ")" << std::endl;
        
//         int serverContentFlag = 0;
        
//         // Parse server content
//         while (peekToken() != "}" && !peekToken().empty()) {
//             if (peekToken() == "location") {
//                 if (!parseLocation(server, serverContentFlag)) {
//                     std::cout << "[ERROR] Failed to parse location block" << std::endl;
//                     return false;
//                 }
//             } else {
//                 if (!parseDirective(server.directives, server.serverFlag, serverContentFlag)) {
//                     std::cout << "[ERROR] Failed to parse server directive" << std::endl;
//                     return false;
//                 }
//             }
//         }
        
//         std::string closeBrace = getNextToken();
//         if (closeBrace != "}") {
//             std::cout << "[ERROR] Expected '}' to close server block, got: " << closeBrace << std::endl;
//             return false;
//         }
        
//         servers.push_back(server);
//         std::cout << "[PARSER] Server block parsed successfully with " 
//                   << server.locations.size() << " locations" << std::endl;
        
//         return true;
//     }
    
//     // Main parsing function
//     bool parse(const std::string& filename) {
//         std::cout << "[PARSER] Starting to parse config file: " << filename << std::endl;
        
//         std::ifstream file(filename.c_str());
//         if (!file.is_open()) {
//             std::cout << "[ERROR] Cannot open config file: " << filename << std::endl;
//             return false;
//         }
        
//         std::string content((std::istreambuf_iterator<char>(file)),
//                            std::istreambuf_iterator<char>());
//         file.close();
        
//         std::cout << "[PARSER] File read successfully (" << content.size() << " bytes)" << std::endl;
        
//         // Remove comments (lines starting with #)
//         std::istringstream iss(content);
//         std::string line;
//         std::string cleanContent;
        
//         while (std::getline(iss, line)) {
//             // Remove leading whitespace
//             size_t start = line.find_first_not_of(" \t");
//             if (start != std::string::npos && line[start] != '#') {
//                 cleanContent += line + "\n";
//             }
//         }
        
//         tokenize(cleanContent);
        
//         currentToken = 0;
//         currentServerFlag = -1;
        
//         // Parse all server blocks
//         while (currentToken < tokens.size()) {
//             if (peekToken() == "server") {
//                 if (!parseServer()) {
//                     std::cout << "[ERROR] Failed to parse server block" << std::endl;
//                     return false;
//                 }
//             } else {
//                 std::cout << "[ERROR] Unexpected token at top level: " << peekToken() << std::endl;
//                 return false;
//             }
//         }
        
//         std::cout << "[PARSER] Parsing completed successfully!" << std::endl;
//         std::cout << "[PARSER] Total servers parsed: " << servers.size() << std::endl;
        
//         return true;
//     }
    
//     // Print parsed configuration
//     void printConfig() {
//         std::cout << "\n=== PARSED CONFIGURATION ===" << std::endl;
        
//         for (size_t i = 0; i < servers.size(); i++) {
//             const Server& server = servers[i];
            
//             std::cout << "\nServer " << server.serverFlag << ":" << std::endl;
            
//             // Print server directives
//             for (std::map<std::string, std::string>::const_iterator it = server.directives.begin();
//                  it != server.directives.end(); ++it) {
//                 std::cout << "  " << it->first << " = " << it->second << std::endl;
//             }
            
//             // Print locations
//             for (size_t j = 0; j < server.locations.size(); j++) {
//                 const Location& loc = server.locations[j];
//                 std::cout << "  Location " << loc.locationFlag << " (" << loc.path << "):" << std::endl;
                
//                 for (std::map<std::string, std::string>::const_iterator it = loc.directives.begin();
//                      it != loc.directives.end(); ++it) {
//                     std::cout << "    " << it->first << " = " << it->second << std::endl;
//                 }
//             }
//         }
        
//         std::cout << "\n=== END CONFIGURATION ===" << std::endl;
//     }
    
//     // Getter for servers
//     const std::vector<Server>& getServers() const {
//         return servers;
//     }
// };

// int main(int argc, char* argv[]) {
//     if (argc != 2) {
//         std::cout << "Usage: " << argv[0] << " <config_file>" << std::endl;
//         std::cout << "Example: " << argv[0] << " test.conf" << std::endl;
//         return 1;
//     }
    
//     ConfigParser parser;
//     std::string configFile = argv[1];
    
//     std::cout << "=== WEBSERVER CONFIG PARSER ===" << std::endl;
//     std::cout << "Starting configuration parsing for: " << configFile << std::endl;
    
//     if (parser.parse(configFile)) {
//         std::cout << "\n[SUCCESS] Configuration file parsed successfully!" << std::endl;
//         parser.printConfig();
//     } else {
//         std::cout << "\n[FAILURE] Failed to parse configuration file: " << configFile << std::endl;
//         return 1;
//     }
    
//     return 0;
// }

int main()
{
    // std::cout << "🎬 Starting Epoll Web Server..." << std::endl;
    // std::cout << "📚 This demonstrates epoll multiplexing in C++98" << std::endl;
    // std::cout << "⚡ Can handle thousands of concurrent connections!\n" << std::endl;
    
    EpollServer server;
    server.run();
    
    std::cout << "\n👋 Server shutting down..." << std::endl;
    return 0;
}

// int main(int ac, char *av[])
// {
//     if (ac > 2) {
//         std::cerr << "Usage: " << av[0] << " <config_file>" << std::endl;
//         return 1;
//     }
    
//     return 0;
// }