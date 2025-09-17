#include "utils.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <dirent.h>

bool fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool isDirectory(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0)
        return false;
    return S_ISDIR(buffer.st_mode);
}

bool isReadable(const std::string& path) {
    // Test più accurato: prova ad aprire il file in lettura
    std::ifstream file(path.c_str());
    bool result = file.good();
    file.close();
    return result;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Impossibile aprire il file: " + path);
    }
    
    // Ottieni la dimensione del file
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Controlla dimensione file valida
    if (size < 0) {
        throw std::runtime_error("Errore determinazione dimensione file");
    }
    
    // Alloca buffer e leggi il file
    std::vector<char> buffer(size);
    if (!file.read(&buffer[0], size)) {
        throw std::runtime_error("Errore lettura file");
    }
    
    return std::string(buffer.begin(), buffer.end());
}

std::string joinPaths(const std::string& base, const std::string& rel) {
    if (base.empty())
        return rel;
    if (rel.empty())
        return base;
    
    std::string result = base;
    if (result[result.length() - 1] != '/' && rel[0] != '/')
        result += '/';
    else if (result[result.length() - 1] == '/' && rel[0] == '/')
        result.erase(result.length() - 1);
    result += rel;
    return normalizePath(result);
}

std::string normalizePath(const std::string& path) {
    std::vector<std::string> parts;
    std::istringstream iss(path);
    std::string part;
    bool isAbsolute = !path.empty() && path[0] == '/';
    
    // Separa il path in componenti
    while (!iss.eof()) {
        std::getline(iss, part, '/');
        if (part.empty() || part == ".")
            continue;
        else if (part == "..") {
            if (!parts.empty() && parts.back() != "..")
                parts.pop_back();
            else if (!isAbsolute)  // Solo se non è un path assoluto
                parts.push_back("..");
        } else {
            parts.push_back(part);
        }
    }
    
    // Ricostruisci il path
    std::ostringstream normalized;
    if (isAbsolute)
        normalized << "/";
        
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0)
            normalized << "/";
        normalized << parts[i];
    }
    
    std::string result = normalized.str();
    if (result.empty() && isAbsolute)
        return "/";
    return result;
}

std::vector<std::string> listDirectory(const std::string& path) {
    std::vector<std::string> result;
    DIR* dir = opendir(path.c_str());
    if (!dir)
        return result;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name != "." && name != "..")
            result.push_back(name);
    }
    
    closedir(dir);
    return result;
}