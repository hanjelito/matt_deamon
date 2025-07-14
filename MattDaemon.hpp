#ifndef MATT_DAEMON_HPP
#define MATT_DAEMON_HPP

#include "Tintin_reporter.hpp"
#include "Server.hpp"
#include <sys/file.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>

#define LOCK_FILE "./matt_daemon.lock"
#define LOG_PATH "./logs/matt_daemon.log"
#define LOG_DIR "./logs"

class MattDaemon {
    private:
        Tintin_reporter _logger;
        Server _server;
        int _lockFd;
        bool _running;
        
        // Métodos específicos de logging del daemon
        void logStart();
        void logCreateServer();
        void logServerCreated();
        void logEnterDaemon();
        void logStarted(int pid);
        void logUserInput(const std::string& input);
        void logRequestQuit();
        void logQuitting();
        void logSignalHandler();
        void logErrorFileLocked();
        
        // Métodos principales del daemon
        bool checkRoot();
        bool createLockFile();
        void removeLockFile();
        bool createLogDirectory();
        void daemonize();
        void setupSignalHandlers();
        void cleanup();
        
        // Callbacks para el servidor
        void onMessageReceived(const std::string& message);
        void onQuitRequested();
        
    public:
        // Forma Coplien (constructor de copia y operador= están disabled)
        MattDaemon();
        MattDaemon(const MattDaemon& other) = delete;
        MattDaemon& operator=(const MattDaemon& other) = delete;
        ~MattDaemon();
        
        // Método principal
        int run();
        void stop();
        
        // Para verificar estado
        bool isRunning() const;
};

#endif