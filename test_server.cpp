#include "Server.hpp"
#include <iostream>
#include <signal.h>

Server* server_instance = nullptr;

void signalHandler(int) {
    if (server_instance) {
        server_instance->stop();
    }
    exit(0);
}

int main() {
    Server server;
    server_instance = &server;
    
    // Configurar manejador de señales
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Configurar callbacks
    server.setMessageHandler([](const std::string& message) {
        std::cout << "Mensaje recibido: " << message << std::endl;
    });
    
    server.setQuitHandler([]() {
        std::cout << "Cliente solicita quit" << std::endl;
    });
    
    // Crear y iniciar servidor
    if (!server.create()) {
        std::cerr << "Error creando servidor" << std::endl;
        return 1;
    }
    
    std::cout << "Servidor iniciado en puerto 4242" << std::endl;
    std::cout << "Usa 'telnet localhost 4242' para conectar" << std::endl;
    std::cout << "Ctrl+C para terminar" << std::endl;
    
    server.start();
    
    return 0;
}