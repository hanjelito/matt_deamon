#include "MattDaemon.hpp"
#include <iostream>
#include <stdexcept>
#include <cerrno>
#include <cstring>

// Variable global para el manejador de señales
static volatile sig_atomic_t g_stop_daemon = 0;

// Manejador de señales (async-signal-safe)
void signalHandler(int signum) {
    (void)signum; // Evitar warning
    g_stop_daemon = 1;
}

// Constructor por defecto
MattDaemon::MattDaemon() : _logger(""), _lockFd(-1), _running(false) {
}


// Destructor
MattDaemon::~MattDaemon() {
    cleanup();
}

// Verificar permisos de root
bool MattDaemon::checkRoot() {
    if (getuid() != 0) {
        std::cerr << "Warning: Matt_daemon should be run as root" << std::endl;
        // return false; // Comentado para testing en Mac
    }
    return true;
}

// Crear archivo de bloqueo
bool MattDaemon::createLockFile() {
    _lockFd = open(LOCK_FILE, O_CREAT | O_RDWR, 0644);
    if (_lockFd == -1) {
        std::cerr << "Can't create lock file: " << LOCK_FILE << std::endl;
        return false;
    }
    
    if (flock(_lockFd, LOCK_EX | LOCK_NB) == -1) {
        std::cerr << "Can't lock: " << LOCK_FILE << std::endl;
        close(_lockFd);
        _lockFd = -1;
        return false;
    }
    return true;
}

// Eliminar archivo de bloqueo
void MattDaemon::removeLockFile() {
    if (_lockFd != -1) {
        flock(_lockFd, LOCK_UN);
        close(_lockFd);
        unlink(LOCK_FILE);
        _lockFd = -1;
    }
}

// Crear directorio de logs
bool MattDaemon::createLogDirectory() {
    struct stat st;
    if (stat(LOG_DIR, &st) == -1) {
        if (mkdir(LOG_DIR, 0755) == -1) {
            std::cerr << "Failed to create log directory: " << strerror(errno) << std::endl;
            return false;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        std::cerr << LOG_DIR << " exists but is not a directory" << std::endl;
        return false;
    }
    return true;
}

// Daemonizar el proceso
void MattDaemon::daemonize() {
    pid_t pid = fork();
    
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }
    
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    
    pid = fork();
    
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    umask(0);
    chdir("/");
    
    // Cerrar descriptores de archivo estándar
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    logEnterDaemon();
    logStarted(getpid());
}

// Configurar manejadores de señales
void MattDaemon::setupSignalHandlers() {
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGQUIT, signalHandler);
}

// Limpiar recursos
void MattDaemon::cleanup() {
    _server.stop();
    removeLockFile();
}

// Callback para mensajes recibidos
void MattDaemon::onMessageReceived(const std::string& message) {
    logUserInput(message);
}

// Callback para quit solicitado
void MattDaemon::onQuitRequested() {
    logRequestQuit();
    _running = false;
}

// Verificar si está ejecutándose
bool MattDaemon::isRunning() const {
    return _running;
}

// Detener el daemon
void MattDaemon::stop() {
    logSignalHandler();
    _running = false;
    _server.stop();
}

// Función principal del daemon
int MattDaemon::run() {
    try {
        // Verificar permisos de root
        if (!checkRoot()) {
            return 1;
        }
        
        // Crear directorio de logs
        if (!createLogDirectory()) {
            return 1;
        }
        
        // Inicializar logger después de crear directorio
        _logger = Tintin_reporter(LOG_PATH);
        if (!_logger.isOpen()) {
            std::cerr << "Failed to open log file: " << LOG_PATH << std::endl;
            return 1;
        }
        
        logStart();
        
        // Crear archivo de bloqueo
        if (!createLockFile()) {
            logErrorFileLocked();
            logQuitting();
            return 1;
        }
        
        // Crear servidor
        logCreateServer();
        if (!_server.create()) {
            _logger.error("Matt_daemon: Failed to create server.");
            logQuitting();
            return 1;
        }
        logServerCreated();
        
        // Configurar callbacks del servidor
        _server.setMessageHandler([this](const std::string& msg) {
            this->onMessageReceived(msg);
        });
        
        _server.setQuitHandler([this]() {
            this->onQuitRequested();
        });
        
        // Daemonizar
        daemonize();
        
        // Configurar manejadores de señales
        setupSignalHandlers();
        
        _running = true;
        
        // Marcar servidor como running también
        _server.setRunning(true);
        
        // Bucle principal del daemon - maneja servidor y señales
        while (_running && !g_stop_daemon) {
            // Verificar señales
            if (g_stop_daemon) {
                logSignalHandler();
                _running = false;
                _server.stop();
                break;
            }
            
            // Procesar una iteración del servidor (no bloqueante)
            _server.handleOneIteration();
            
            // Pequeña pausa para no consumir CPU innecesariamente
            usleep(50000); // 50ms
        }
        
        // Si salimos por señal y no por quit del cliente
        if (g_stop_daemon && _running) {
            logSignalHandler();
        }
        
        logQuitting();
        return 0;
        
    } catch (const std::exception& e) {
        _logger.error("Matt_daemon: " + std::string(e.what()));
        logQuitting();
        return 1;
    }
}

// Métodos específicos de logging
void MattDaemon::logStart() {
    _logger.info("Matt_daemon: Started.");
}

void MattDaemon::logCreateServer() {
    _logger.info("Matt_daemon: Creating server.");
}

void MattDaemon::logServerCreated() {
    _logger.info("Matt_daemon: Server created.");
}

void MattDaemon::logEnterDaemon() {
    _logger.info("Matt_daemon: Entering Daemon mode.");
}

void MattDaemon::logStarted(int pid) {
    _logger.info("Matt_daemon: started. PID: " + std::to_string(pid) + ".");
}

void MattDaemon::logUserInput(const std::string& input) {
    _logger.log("Matt_daemon: User input: " + input);
}

void MattDaemon::logRequestQuit() {
    _logger.info("Matt_daemon: Request quit.");
}

void MattDaemon::logQuitting() {
    _logger.info("Matt_daemon: Quitting.");
}

void MattDaemon::logSignalHandler() {
    _logger.info("Matt_daemon: Signal handler.");
}

void MattDaemon::logErrorFileLocked() {
    _logger.error("Matt_daemon: Error file locked.");
}