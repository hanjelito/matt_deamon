#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <vector>
#include <functional>

#define PORT 4242
#define MAX_CLIENTS 3

class Server {
    private:
        int _serverSocket;
        std::vector<int> _clientSockets;
        bool _running;

        std::function<void(const std::string&)> _messageHandler;
        std::function<void()> _quitHandler;

    public:
        Server();
        Server(const Server& other);
        Server& operator=(const Server& other);
        ~Server();

        bool create();
        void start();
        void stop();
        bool isRunning() const;

        // esto es para los callback
        void setMessageHabdler(std::function<void(const std::string&)> handler);
        void setQuitHandler(std::function<void()> handler);

        //
        int getSocketFd() const;
        size_t getClientCount() const;

    private:
        void handleConnections();
        void hangleClient(int clientSocket);
        void cleanup();
        bool acceptNewClient();
        void processExistingClients(fd_set& readSet);

};
#endif
