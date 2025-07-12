#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno> 

class Daemon {
    public:
        Daemon() = default;
        Daemon(const Daemon& other) = default;
        Daemon &operator=(const Daemon& other) = default;
        ~Daemon() = default;
        
        bool daemonize() {
            pid_t pid = fork();
            if ( pid < 0 ) return false;
            if ( pid > 0 ) _exit(EXIT_SUCCESS);
            
            if ( setsid() < 0 ) return false;
            
            pid = fork();
            if ( pid < 0 ) return false;
            if ( pid > 0 ) _exit(EXIT_SUCCESS);

            umask(0);
            chdir("/");

            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            // close(STDERR_FILENO);

            open("/dev/null", O_RDONLY); // stdin
            open("/dev/null", O_RDWR); // stdout
            // open("/dev/null", O_RDWR); // stderr

            return true;
        }

        void run() {
            while(true) {
                std::cerr << "test" << std::endl; 
                sleep(10);
            }
        }
};


int main() {
    Daemon d;
    if (!d.daemonize()) {
        std::cerr << "Error al demonizar: " << std::strerror(errno) << "\n";
        return EXIT_FAILURE;
    }
    d.run();
    return EXIT_SUCCESS;
}
