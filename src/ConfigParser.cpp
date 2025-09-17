// ********** CONFIG_PARSER **********
// Implementazione parser di configurazione

#include "ConfigParser.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cctype>

// Helper compatibile C++98 per convertire numeri in stringa
template <typename T>
std::string to_string98(T n) {
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

// Costruttore: salva path
ConfigParser::ConfigParser(const std::string& path) : _path(path) {}

// Avvia parsing
void ConfigParser::parse()
{
    _readFile();
    _cleanLines();
    _parseBlocks();
}

// Ritorna servers
const std::vector<ServerConfig>& ConfigParser::getServers() const {
    return _servers;
}

// Ritorna lista host:port per bind
std::vector< std::pair<std::string,int> >
ConfigParser::getListenList() const {
    std::vector< std::pair<std::string,int> > list;
    for (size_t i = 0; i < _servers.size(); ++i) {
        for (size_t j = 0; j < _servers[i].listen.size(); ++j)
            list.push_back(_servers[i].listen[j]);
    }
    return list;
}

// Legge righe file
void ConfigParser::_readFile()
{
    std::ifstream file(_path.c_str());
    if (!file.is_open()) {
        throw ConfigException("Impossibile aprire il file: " + _path);
    }
    std::string line;
    while (std::getline(file, line))
        _rawLines.push_back(line);
    file.close();
}

// Rimuove commenti e spazi
void ConfigParser::_cleanLines()
{
    std::vector<std::string> clean;
    for (size_t i = 0; i < _rawLines.size(); ++i) {
        std::string line = _rawLines[i];

        size_t comment = line.find('#');
        if (comment != std::string::npos)
            line = line.substr(0, comment);

        _trim(line);
        if (!line.empty())
            clean.push_back(line);
    }
    _rawLines = clean;
}

// Isola blocchi server
void ConfigParser::_parseBlocks()
{
    std::vector<std::string> current;
    bool inBlock = false;
    int braceCount = 0;
    size_t blockStartLine = 0;

    for (size_t i = 0; i < _rawLines.size(); ++i) {
        const std::string& line = _rawLines[i];

        if (!inBlock && _startsWith(line, "server")
            && line.find('{') != std::string::npos) {
            inBlock = true;
            braceCount = 1;
            current.push_back(line);
            blockStartLine = i + 1;
            continue;
        }

        if (inBlock) {
            if (line.find('{') != std::string::npos)
                braceCount++;
            if (line.find('}') != std::string::npos)
                braceCount--;
            current.push_back(line);

            if (braceCount == 0) {
                try {
                    ServerConfig srv = _parseServerBlock(current, blockStartLine);
                    _servers.push_back(srv);
                } catch (const ConfigException& e) {
                    throw; // Propaga l'errore con il messaggio già pronto
                }
                current.clear();
                inBlock = false;
            }
        }
    }
}

// Parsers di un blocco server
ServerConfig ConfigParser::_parseServerBlock(
    const std::vector<std::string>& block, size_t blockStartLine)
{
    ServerConfig srv;
    srv.client_max_body_size = 0;

    std::vector<std::string> currentLoc;
    bool inLoc = false;
    int braceCount = 0;
    size_t lineInFile = blockStartLine;

    for (size_t i = 1; i < block.size() - 1; ++i, ++lineInFile) {
        std::string line = block[i];

        if (!inLoc && _startsWith(line, "location")) {
            inLoc = true;
            braceCount = 1;
            currentLoc.push_back(line);
            continue;
        }

        if (inLoc) {
            if (line.find('{') != std::string::npos)
                braceCount++;
            if (line.find('}') != std::string::npos)
                braceCount--;
            currentLoc.push_back(line);

            if (braceCount == 0) {
                try {
                    LocationConfig loc = _parseLocationBlock(currentLoc, lineInFile - currentLoc.size() + 1);
                    srv.locations.push_back(loc);
                } catch (const ConfigException& e) {
                    throw;
                }
                currentLoc.clear();
                inLoc = false;
            }
            continue;
        }

        if (_startsWith(line, "listen"))
            _parseListenLine(line, srv, lineInFile);
        else if (_startsWith(line, "server_name")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            std::string tmp, value;
            iss >> tmp >> value;
            if (value.empty())
                throw ConfigException("Invalid server_name at line " + to_string98(lineInFile) + ": missing name");
            srv.server_name = value;
        }
        else if (_startsWith(line, "root")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            std::string tmp, value;
            iss >> tmp >> value;
            if (value.empty())
                throw ConfigException("Invalid root directive at line " + to_string98(lineInFile) + ": missing path");
            srv.root = value;
        }
        else if (_startsWith(line, "error_page"))
            _parseErrorPageLine(line, srv, lineInFile);
        else if (_startsWith(line, "client_max_body_size")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            std::string tmp;
            size_t val;
            iss >> tmp >> val;
            srv.client_max_body_size = val;
        }
				// ...dopo aver processato tutte le direttive...
		if (srv.listen.empty())
			throw ConfigException("Missing listen directive in server block");
    }
    return srv;
}

// Parser di un blocco location
LocationConfig ConfigParser::_parseLocationBlock(
    const std::vector<std::string>& block, size_t blockStartLine)
{
    LocationConfig loc;
    loc.autoindex = false;
    loc.max_body_size = 0;

    std::istringstream first(block[0]);
    std::string tmp;
    first >> tmp >> loc.path;
    if (loc.path.empty())
        throw ConfigException("Invalid location path at line " + to_string98(blockStartLine));

    for (size_t i = 1; i < block.size() - 1; ++i) {
        std::string line = block[i];

        if (_startsWith(line, "root")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            iss >> tmp >> loc.root;
            if (loc.root.empty())
                throw ConfigException("Invalid root in location at line " + to_string98(blockStartLine + i) + ": missing path");
        }
        else if (_startsWith(line, "index")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            iss >> tmp >> loc.index;
        }
        else if (_startsWith(line, "autoindex")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            std::string val;
            iss >> tmp >> val;
            loc.autoindex = (val == "on");
        }
        else if (_startsWith(line, "limit_except")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            iss >> tmp;
            std::string method;
            while (iss >> method)
                loc.methods.push_back(method);
        }
        else if (_startsWith(line, "upload_store")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            iss >> tmp >> loc.upload_dir;
        }
        else if (_startsWith(line, "cgi_pass")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            std::string ext, path;
            iss >> tmp >> ext >> path;
            loc.cgi[ext] = path;
        }
        else if (_startsWith(line, "return")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            iss >> tmp >> loc.redirect;
        }
        else if (_startsWith(line, "client_max_body_size")) {
            _stripSemicolon(line);
            std::istringstream iss(line);
            size_t val;
            iss >> tmp >> val;
            loc.max_body_size = val;
        }
    }
    return loc;
}

// Parser di una direttiva listen con validazione
void ConfigParser::_parseListenLine(
    const std::string& line, ServerConfig &srv, size_t lineNum)
{
    std::string copy = line;
    _stripSemicolon(copy);
    std::istringstream iss(copy);
    std::string tmp, val;
    iss >> tmp >> val;

    size_t colon = val.find(':');
    std::string host = "0.0.0.0";
    int port = 80;

    if (colon != std::string::npos) {
        host = val.substr(0, colon);
        std::string port_str = val.substr(colon+1);
        if (host.empty() || port_str.empty())
            throw ConfigException("Invalid listen directive at line " + to_string98(lineNum) + ": " + val);
        char* endptr = NULL;
        long port_l = std::strtol(port_str.c_str(), &endptr, 10);
        if (*endptr != '\0' || port_l < 1 || port_l > 65535)
            throw ConfigException("Invalid listen directive at line " + to_string98(lineNum) + ": " + val);
        port = static_cast<int>(port_l);
    } else {
        // Solo porta
        if (val.empty())
            throw ConfigException("Invalid listen directive at line " + to_string98(lineNum) + ": missing port");
        char* endptr = NULL;
        long port_l = std::strtol(val.c_str(), &endptr, 10);
        if (*endptr != '\0' || port_l < 1 || port_l > 65535)
            throw ConfigException("Invalid listen directive at line " + to_string98(lineNum) + ": " + val);
        port = static_cast<int>(port_l);
    }
    srv.listen.push_back(std::make_pair(host, port));
}

// Parser di una error_page
void ConfigParser::_parseErrorPageLine(
    const std::string& line, ServerConfig &srv, size_t lineNum)
{
    std::string copy = line;
    _stripSemicolon(copy);
    std::istringstream iss(copy);
    std::string tmp, path;
    int code;
    iss >> tmp >> code >> path;
    if (iss.fail() || path.empty())
        throw ConfigException("Invalid error_page directive at line " + to_string98(lineNum));
    srv.error_pages[code] = path;
}

// Trim spazi
void ConfigParser::_trim(std::string &s)
{
    while (!s.empty() && isspace(s[0]))
        s.erase(0,1);
    while (!s.empty() && isspace(s[s.size()-1]))
        s.erase(s.size()-1);
}

// Rimuove ';'
void ConfigParser::_stripSemicolon(std::string &s)
{
    if (!s.empty() && s[s.size()-1] == ';')
        s.erase(s.size()-1);
}

// Check prefisso
bool ConfigParser::_startsWith(
    const std::string &s, const std::string &pref)
{
    return (s.compare(0, pref.size(), pref) == 0);
}
