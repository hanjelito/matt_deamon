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

#define LOCK_FILE "/var/lock/matt_daemon.lock"
#define LOG_PATH "/var/log/matt_daemon/matt_daemon.log"
#define LOG_DIR "/var/log/matt_daemon"

class MattDaemon {
    private:
        Tintin_reporter _logger;
        Server _server;
        int _lockFd;
        bool _running;


        // Metodos especificos de loggin del daemon
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


        //Metodos principales del daemon
        bool checkRoot();
        bool createLockFile();
        void removeLogFile();
        bool createLogDirectory();
        void daemonize();
        void setupSignalHandlers();
        void cleanup();

        //callback para el servidor
        void onMessageReceived(const std::string& message);
        void onQuitRequested();
        
    public:
        MattDaemon();
        MattDaemon(const MattDaemon& other);
        MattDaemon& operator=(const MattDaemon& other);
        ~MattDaemon();

        // Metodo principal
        int run();
        void stop();
        // para el manejador de señales
        bool isRunning() const;
};
#endif
