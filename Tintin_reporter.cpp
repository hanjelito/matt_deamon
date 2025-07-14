#include "Tintin_reporter.hpp"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <sys/stat.h>
#include <iostream>

// Constructor por defecto
Tintin_reporter::Tintin_reporter() : _logPath("") {
}

// Constructor con ruta de log
Tintin_reporter::Tintin_reporter(const std::string& logPath) : _logPath(logPath) {
    if (!_logPath.empty()) {
        _logFile.open(_logPath.c_str(), std::ios::out | std::ios::app);
        if (!_logFile.is_open()) {
            std::cerr << "Warning: Could not open log file: " << _logPath << std::endl;
        }
    }
}

// Constructor de copia
Tintin_reporter::Tintin_reporter(const Tintin_reporter& other) : _logPath(other._logPath) {
    if (!_logPath.empty()) {
        _logFile.open(_logPath.c_str(), std::ios::out | std::ios::app);
        if (!_logFile.is_open()) {
            std::cerr << "Warning: Could not open log file: " << _logPath << std::endl;
        }
    }
}

// Operador de asignación
Tintin_reporter& Tintin_reporter::operator=(const Tintin_reporter& other) {
    if (this != &other) {
        // Cerrar archivo actual si está abierto
        if (_logFile.is_open()) {
            _logFile.close();
        }
        
        _logPath = other._logPath;
        
        // Abrir nuevo archivo si hay ruta
        if (!_logPath.empty()) {
            _logFile.open(_logPath.c_str(), std::ios::out | std::ios::app);
            if (!_logFile.is_open()) {
                std::cerr << "Warning: Could not open log file: " << _logPath << std::endl;
            }
        }
    }
    return *this;
}

// Destructor
Tintin_reporter::~Tintin_reporter() {
    close();
}

// Obtener timestamp actual en formato requerido
std::string Tintin_reporter::getCurrentTimestamp() const {
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[32];
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    // Formato: [DD/MM/YYYY-HH:MM:SS]
    strftime(buffer, sizeof(buffer), "[%d/%m/%Y-%H:%M:%S]", timeinfo);
    
    return std::string(buffer);
}

// Escribir al log con formato consistente
void Tintin_reporter::writeToLog(const std::string& level, const std::string& message) {
    if (!_logFile.is_open() || _logPath.empty()) {
        // Silenciosamente no hacer nada si el logger no está inicializado
        return;
    }
    
    std::string timestamp = getCurrentTimestamp();
    std::string logEntry = timestamp + " [ " + level + " ] - " + message;
    
    _logFile << logEntry << std::endl;
    _logFile.flush(); // Asegurar que se escriba inmediatamente
}

// Método de logging nivel INFO
void Tintin_reporter::info(const std::string& message) {
    writeToLog("INFO", message);
}

// Método de logging nivel LOG
void Tintin_reporter::log(const std::string& message) {
    writeToLog("LOG", message);
}

// Método de logging nivel ERROR
void Tintin_reporter::error(const std::string& message) {
    writeToLog("ERROR", message);
}

// Verificar si el archivo está abierto
bool Tintin_reporter::isOpen() const {
    return _logFile.is_open();
}

// Cerrar el archivo de log
void Tintin_reporter::close() {
    if (_logFile.is_open()) {
        _logFile.close();
    }
}