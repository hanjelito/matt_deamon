#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <vector>
#include <fstream>

class Daemon {
private:
    std::vector<pid_t> children;
    static Daemon* instance;
    pid_t daemon_pid;

    static void signal_handler(int sig) {
        if (instance) {
            instance->handle_signal(sig);
        }
    }

    static void sigchld_handler(int sig) {
        if (instance) {
            instance->handle_child_death(sig);
        }
    }

    void handle_signal(int sig) {
        switch (sig) {
        case SIGUSR1:
            std::cerr << "[DAEMON] Recibida señal SIGUSR1 - Creando proceso hijo\n";
            create_child();
            break;
        case SIGUSR2:
            std::cerr << "[DAEMON] Recibida señal SIGUSR2 - Terminando todos los hijos\n";
            kill_all_children();
            break;
        case SIGTERM:
            std::cerr << "[DAEMON] Recibida señal SIGTERM - Terminando daemon\n";
            cleanup_and_exit();
            break;
        case SIGINT:
            std::cerr << "[DAEMON] Recibida señal SIGINT - Terminando daemon\n";
            cleanup_and_exit();
            break;
        default:
            std::cerr << "[DAEMON] Señal no manejada: " << sig << "\n";
        }
    }

    void handle_child_death(int sig) {
        (void)sig;
        pid_t pid;
        int status;

        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            std::cerr << "[DAEMON] Proceso hijo " << pid << " terminado\n";

            for (auto it = children.begin(); it != children.end(); ++it) {
                if (*it == pid) {
                    children.erase(it);
                    break;
                }
            }
            std::cerr << "[DAEMON] Hijos activos restantes: " << children.size() << "\n";
        }
    }

    void create_child() {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "[DAEMON] Error al crear proceso hijo: " << std::strerror(errno) << "\n";
            return;
        }

        if (pid == 0) {
            child_process();
        } else {
            children.push_back(pid);
            std::cerr << "[DAEMON] Proceso hijo creado con PID: " << pid << "\n";
            std::cerr << "[DAEMON] Total de hijos activos: " << children.size() << "\n";
        }
    }

    void child_process() {
        pid_t child_pid = getpid();
        std::cerr << "[HIJO " << child_pid << "] Iniciado\n";

        int counter = 0;
        while (counter < 20) {
            std::cerr << "[HIJO " << child_pid << "] Trabajando... iteracion " << counter << "\n";
            sleep(2);
            counter++;
        }

        std::cerr << "[HIJO " << child_pid << "] Terminando\n";
        _exit(EXIT_SUCCESS);
    }

    void kill_all_children() {
        for (pid_t child_pid : children) {
            std::cerr << "[DAEMON] Terminando proceso hijo: " << child_pid << "\n";
            kill(child_pid, SIGTERM);
        }

        sleep(1);

        for (pid_t child_pid : children) {
            if (kill(child_pid, 0) == 0) {
                std::cerr << "[DAEMON] Forzando terminacion del proceso hijo: " << child_pid << "\n";
                kill(child_pid, SIGKILL);
            }
        }

        children.clear();
    }

    void cleanup_and_exit() {
        std::cerr << "[DAEMON] Iniciando limpieza...\n";
        kill_all_children();

        unlink("/tmp/daemon.pid");

        std::cerr << "[DAEMON] Terminando\n";
        exit(EXIT_SUCCESS);
    }

    void write_pid_file() {
        std::ofstream pid_file("/tmp/daemon.pid");
        if (pid_file.is_open()) {
            pid_file << getpid() << std::endl;
            pid_file.close();
            std::cerr << "[DAEMON] PID guardado en /tmp/daemon.pid\n";
        } else {
            std::cerr << "[DAEMON] Error al crear archivo PID\n";
        }
    }

public:
    Daemon() : daemon_pid(0) {
        instance = this;
    }

    ~Daemon() {
        instance = nullptr;
    }

    bool daemonize() {
        pid_t pid = fork();
        if (pid < 0) return false;
        if (pid > 0) _exit(EXIT_SUCCESS);

        if (setsid() < 0) return false;

        pid = fork();
        if (pid < 0) return false;
        if (pid > 0) _exit(EXIT_SUCCESS);

        daemon_pid = getpid();
        umask(0);
        chdir("/");

        close(STDIN_FILENO);
        close(STDOUT_FILENO);

        open("/dev/null", O_RDONLY);
        open("/dev/null", O_RDWR);

        return true;
    }

    void setup_signals() {
        signal(SIGUSR1, signal_handler);
        signal(SIGUSR2, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGINT, signal_handler);
        signal(SIGCHLD, sigchld_handler);

        signal(SIGHUP, SIG_IGN);
        signal(SIGPIPE, SIG_IGN);
    }

    void run() {
        write_pid_file();
        setup_signals();

        std::cerr << "[DAEMON] Iniciado con PID: " << getpid() << "\n";
        std::cerr << "[DAEMON] Comandos disponibles:\n";
        std::cerr << "[DAEMON]   kill -USR1 " << getpid() << " # crear proceso hijo\n";
        std::cerr << "[DAEMON]   kill -USR2 " << getpid() << " # terminar todos los hijos\n";
        std::cerr << "[DAEMON]   kill -TERM " << getpid() << " # terminar daemon\n";

        while (true) {
            std::cerr << "[DAEMON] Activo - Hijos: " << children.size() << "\n";
            sleep(10);
        }
    }
};

Daemon* Daemon::instance = nullptr;

int main() {
    Daemon d;
    if (!d.daemonize()) {
        std::cerr << "Error al demonizar: " << std::strerror(errno) << "\n";
        return EXIT_FAILURE;
    }
    d.run();
    return EXIT_SUCCESS;
}