// ********** CONFIG_PARSER_HPP **********
// Definizione della classe ConfigParser e delle eccezioni

#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

// ********** LOCATION_CONFIG **********
// Rappresenta un blocco location
struct LocationConfig {
    std::string path;
    std::string root;
    std::string index;
    bool autoindex;
    std::vector<std::string> methods;
    std::string upload_dir;
    std::map<std::string, std::string> cgi;
    std::string redirect;
    size_t max_body_size;
};

// ********** SERVER_CONFIG **********
// Rappresenta un blocco server
struct ServerConfig {
    std::vector< std::pair<std::string,int> > listen;
    std::string server_name;
    std::string root;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size;
    std::vector<LocationConfig> locations;
};

// ********** CONFIG_EXCEPTION **********
// Eccezione personalizzata per errori di parsing
class ConfigException : public std::runtime_error {
    public:
        ConfigException(const std::string& msg)
            : std::runtime_error(msg) {}
};

class ConfigParser {
    public:
        // Costruttore con path del file
        ConfigParser(const std::string& path);

        // Avvia il parsing
        void parse();

        // Ritorna la lista dei server
        const std::vector<ServerConfig>& getServers() const;

        // Lista di host:port per il bind
        std::vector< std::pair<std::string,int> > getListenList() const;

    private:
        std::string _path;
        std::vector<std::string> _rawLines;
        std::vector<ServerConfig> _servers;

        // Legge le righe del file
        void _readFile();

        // Rimuove commenti e vuoti
        void _cleanLines();

        // Isola i blocchi server
        void _parseBlocks();

        // Parsers interni
        ServerConfig _parseServerBlock(
            const std::vector<std::string>& block, size_t blockStartLine);
        LocationConfig _parseLocationBlock(
            const std::vector<std::string>& block, size_t blockStartLine);
        void _parseListenLine(const std::string& line,
            ServerConfig &srv, size_t lineNum);
        void _parseErrorPageLine(const std::string& line,
            ServerConfig &srv, size_t lineNum);

        // Helpers stringa
        void _trim(std::string &s);
        void _stripSemicolon(std::string &s);
        bool _startsWith(const std::string &s,
            const std::string &pref);
};

#endif
