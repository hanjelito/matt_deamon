#include "Server.hpp"
#include <iostream>
#include <cerrno>
#include <stdexcept>

// Constructor por defecto: inicializa socket en -1 (inválido) y estado no ejecutándose
Server::Server(): _serverSocket(-1), _running(false) {}

// Constructor de copia: crea nueva instancia sin copiar sockets activos
Server::Server(const Server& other) : _serverSocket(-1), _running(false) {
    *this = other;
}

// Operador de asignación: limpia recursos actuales y copia solo los handlers
Server& Server::operator=(const Server& other) {
    if (this != &other) {
        cleanup();  // Cierra sockets y libera recursos
        _serverSocket = -1;
        _running = false;
        // Solo copia los handlers, no los sockets activos
        _messageHandler = other._messageHandler;
        _quitHandler = other._quitHandler;
    }
    return *this;
}

// Destructor: garantiza limpieza de recursos al destruir la instancia
Server::~Server() {
    cleanup();
}

// Crea y configura el socket del servidor
bool Server::create() {
    // 1. Crear socket TCP/IP
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket == -1) {
        return false;
    }

    // 2. Configurar reutilización de dirección (evita "Address already in use")
    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(_serverSocket);
        _serverSocket = -1;
        return false;
    }

    // 3. Configurar dirección del servidor
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;              // IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY;      // Escucha en todas las interfaces
    serverAddr.sin_port = htons(PORT);            // Puerto en formato de red

    // 4. Asociar socket con la dirección y puerto
    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(_serverSocket);
        _serverSocket = -1;
        return false;
    }

    // 5. Configurar socket para escuchar conexiones entrantes
    if (listen(_serverSocket, MAX_CLIENTS) == -1) {
        close(_serverSocket);
        _serverSocket = -1;
        return false;
    }
    return true;
}

// Inicia el servidor y el bucle principal de manejo de conexiones
void Server::start() {
    if (_serverSocket == -1) {
        throw std::runtime_error("Server not created");
    }
    _running = true;
    handleConnections();  // Entra en el bucle principal
}

// Marca el servidor para detención (el bucle principal terminará)
void Server::stop() {
    _running = false;
}

// Establecer estado de running
void Server::setRunning(bool running) {
    _running = running;
}

// Verifica si el servidor está ejecutándose
bool Server::isRunning() const {
    return _running;
}

// Configura función callback para procesar mensajes recibidos
void Server::setMessageHandler(std::function<void(const std::string&)> handler) {
    _messageHandler = handler;
}

// Configura función callback para manejar comando "quit"
void Server::setQuitHandler(std::function<void()> handler) {
    _quitHandler = handler;
}

// Obtiene el file descriptor del socket del servidor
int Server::getSocketFd() const {
    return _serverSocket;
}

// Retorna el número de clientes conectados actualmente
size_t Server::getClientCount() const {
    return _clientSockets.size();
}

// Una iteración no bloqueante del servidor
void Server::handleOneIteration() {
    if (_serverSocket == -1 || !_running) {
        return;
    }
    
    fd_set readSet;
    int maxFd = _serverSocket;

    // Preparar conjunto de file descriptors para monitorear
    FD_ZERO(&readSet);
    FD_SET(_serverSocket, &readSet);

    // Agregar todos los sockets de clientes al conjunto
    for (int clientSocket : _clientSockets) {
        FD_SET(clientSocket, &readSet);
        if (clientSocket > maxFd) {
            maxFd = clientSocket;
        }
    }
    
    // Timeout muy corto para no bloquear
    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 10000;  // 10ms
    
    // Esperar actividad en algún socket
    int activity = select(maxFd + 1, &readSet, nullptr, nullptr, &timeout);
    
    if (activity > 0) {
        // Nueva conexión entrante
        if (FD_ISSET(_serverSocket, &readSet)) {
            acceptNewClient();
        }
        
        // Datos de clientes existentes
        processExistingClients(readSet);
    }
}

// Bucle principal del servidor: maneja conexiones usando select()
void Server::handleConnections() {
    fd_set readSet;
    int maxFd = _serverSocket;

    while (_running) {
        // 1. Preparar conjunto de file descriptors para monitorear
        FD_ZERO(&readSet);                    // Limpiar conjunto
        FD_SET(_serverSocket, &readSet);      // Agregar socket servidor

        // 2. Agregar todos los sockets de clientes al conjunto
        maxFd = _serverSocket;
        for (int clientSocket : _clientSockets) {
            FD_SET(clientSocket, &readSet);
            if (clientSocket > maxFd) {
                maxFd = clientSocket;  // Actualizar máximo FD
            }
        }
        
        // 3. Configurar timeout para select() (evita bloqueo indefinido)
        struct timeval timeout;
        timeout.tv_sec  = 1;  // 1 segundo
        timeout.tv_usec = 0;  // 0 microsegundos
        
        // 4. Esperar actividad en algún socket
        int activity = select(maxFd + 1, &readSet, nullptr, nullptr, &timeout);
        
        // 5. Manejar errores (excepto interrupciones)
        if (activity < 0 && errno != EINTR) {
            break;
        }
        
        // 6. Procesar actividad detectada
        if (activity > 0) {
            // Nueva conexión entrante
            if (FD_ISSET(_serverSocket, &readSet)) {
                acceptNewClient();
            }
            
            // Datos de clientes existentes
            processExistingClients(readSet);
        }
    }

}

// Acepta nueva conexión de cliente si hay espacio disponible
bool Server::acceptNewClient() {
    // Verificar límite de clientes
    if (_clientSockets.size() >= MAX_CLIENTS) {
        return false;
    }

    // Aceptar nueva conexión
    struct sockaddr_in clientAdd;
    socklen_t clientLen = sizeof(clientAdd);
    int clientSocket = accept(_serverSocket, (struct sockaddr*)&clientAdd, &clientLen);

    if (clientSocket >= 0) {
        _clientSockets.push_back(clientSocket);  // Agregar a lista de clientes
        return true;
    }

    return false;
}

// Procesa datos de clientes existentes que tienen actividad
void Server::processExistingClients(fd_set& readSet) {
    for (auto it = _clientSockets.begin(); it != _clientSockets.end();) {
        int clientSocket = *it;
        // Verificar si este cliente tiene datos listos para leer
        if (FD_ISSET(clientSocket, &readSet)) {
            if (!handleClient(clientSocket)) {
                // Cliente se desconectó o envió comando "quit"
                close(clientSocket);
                it = _clientSockets.erase(it);  // Remover de lista
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

// Maneja datos recibidos de un cliente específico
bool Server::handleClient(int clientSocket) {
    char buffer[1024];
    // Leer datos del cliente (no bloqueante)
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    
    if (bytesRead > 0) {
        // Datos recibidos exitosamente
        buffer[bytesRead] = '\0';
        std::string message(buffer);

        // Limpiar caracteres de nueva línea al final
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();   
        }

        // Procesar comando "quit" - termina el servidor
        if (message == "quit") {
            if (_quitHandler) {
                _quitHandler();
            }
            _running = false;
            return false; // Cliente se desconecta
        } else if (!message.empty()) {
            // Procesar mensaje normal
            if (_messageHandler) {
                _messageHandler(message);
            }
        }
        return true; // Cliente sigue conectado
    } else if (bytesRead == 0) {
        return false; // Cliente se desconectó limpiamente
    } else {
        // Error o no hay datos disponibles (EAGAIN/EWOULDBLOCK)
        return true; // Cliente sigue conectado
    }
}

// Limpia todos los recursos del servidor
void Server::cleanup() {
    // Cerrar todos los sockets de clientes
    for (int clientSocket : _clientSockets) {
        close(clientSocket);
    }

    // Limpiar lista de clientes
    _clientSockets.clear();

    // Cerrar socket del servidor
    if (_serverSocket != -1) {
        close(_serverSocket);
        _serverSocket = -1;
    }

    // Marcar como no ejecutándose
    _running = false;
}
