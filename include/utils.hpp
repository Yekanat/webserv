#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sstream>
#include <vector>

// Funzioni esistenti
template <typename T>
std::string to_string98(T n) {
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

// Nuove funzioni
bool fileExists(const std::string& path);
bool isDirectory(const std::string& path);
bool isReadable(const std::string& path);
std::string readFile(const std::string& path);
std::string joinPaths(const std::string& base, const std::string& rel);
std::string normalizePath(const std::string& path);
std::vector<std::string> listDirectory(const std::string& path);

#endif