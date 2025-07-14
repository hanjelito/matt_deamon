#include "Server.hpp"
#include <iostream>
#include <cerrno>
#include <stdexcept>

Server::Server(): _serverSocket(-1), _running(false) {}

Server::Server(const Server& other) : _serverSocket(-1), _running(false) {
    *this = other;
}

Server& Server::operator=(const Server& other) {
    if (this != &other) {
        cleanup();
        _serverSocket = -1;
        _running = false;
        _messageHandler = other._messageHandler;
        _quitHandler = other._quitHandler;
    }
    return *this;
}

Server::~Server() {
    cleanup();
}

//Crea nuestro servidor
bool Server::create() {
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket == -1) {
        return false;
    }

    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        close(_serverSocket);
        _serverSocket = -1;
        return false;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(_serverSocket);
        _serverSocket = -1;
        return false;
    }

    if (listen(_serverSocket, MAX_CLIENTS) == -1) {
        close(_serverSocket);
        _serverSocket = -1;
        return false;
    }
    return true;
}

// inicia nuestro servidor
void Server::start() {
    if (_serverSocket == -1) {
        throw std::runtime_error("Server not created");
    }
    _running = true;
    handleConnections();
}

//Detiene el servidor
void Server::stop() {
    _running = false;
}

// verifica si nuestro servidor esta corriendo
bool Server::isRunning() const {
    return _running;
}

// manejador de mensajes
void Server::setMessageHandler(std::function<void(const std::string&)> handler) {
    _messageHandler = handler;
}

void Server::setQuitHandler(std::function<void()> handler) {
    _quitHandler = handler;
}

// Obtiene el file descriptor de nuestro socket
int Server::getSocketFd() const {
    return _serverSocket;
}

// retorna el numero de clientes conectados
size_t Server::getClientCount() const {
    return _clientSockets.size();
}

void Server::handleConnections() {
    fd_set readSet;
    int maxFd = _serverSocket;

    while (_running) {
        FD_ZERO(&readSet);
        FD_SET(_serverSocket, &readSet);

        maxFd = _serverSocket;
        for (int clientSocket : _clientSockets) {
            FD_SET(clientSocket, &readSet);
            if (clientSocket > maxFd) {
                maxFd = clientSocket;
            }
        }
        
        // Timeout para select(): espera máximo 1 segundo antes de volver a iterar
        struct timeval timeout;
        timeout.tv_sec  = 1;  // segundos
        timeout.tv_usec = 0;  // microsegundos
        
        int activity = select(maxFd + 1, &readSet, nullptr, nullptr, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            break;
        }
        
        if (activity > 0) {
            if (FD_ISSET(_serverSocket, &readSet)) {
                acceptNewClient();
            }
            
            processExistingClients(readSet);
        }
    }

}

// aceptar nuevo cliente
bool Server::acceptNewClient() {
    if (_clientSockets.size() >= MAX_CLIENTS) {
        return false;
    }

    struct sockaddr_in clientAdd;
    socklen_t clientLen = sizeof(clientAdd);
    int clientSocket = accept(_serverSocket, (struct sockaddr*)&clientAdd, &clientLen);

    if (clientSocket >= 0) {
        _clientSockets.push_back(clientSocket);
        return true;
    }

    return false;
}

// Procesar cliente existentes
void Server::processExistingClients(fd_set& readSet) {
    for (auto it = _clientSockets.begin(); it != _clientSockets.end();) {
        int clientSocket = *it;
        if (FD_ISSET(clientSocket, &readSet)) {
            if (!handleClient(clientSocket)) {
                // Cliente se desconectó o envió quit
                close(clientSocket);
                it = _clientSockets.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

// manejar cliente individual
bool Server::handleClient(int clientSocket) {
    char buffer[1024];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string message(buffer);

        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();   
        }

        if (message == "quit") {
            if (_quitHandler) {
                _quitHandler();
            }
            _running = false;
            return false; // Cliente se desconecta
        } else if (!message.empty()) {
            if (_messageHandler) {
                _messageHandler(message);
            }
        }
        return true; // Cliente sigue conectado
    } else if (bytesRead == 0) {
        return false; // Cliente se desconectó
    } else {
        // Error o no hay datos (EAGAIN/EWOULDBLOCK)
        return true; // Cliente sigue conectado
    }
}

void Server::cleanup() {
    for (int clientSocket : _clientSockets) {
        close(clientSocket);
    }

    _clientSockets.clear();

    if (_serverSocket != -1) {
        close(_serverSocket);
        _serverSocket = -1;
    }

    _running = false;
}
