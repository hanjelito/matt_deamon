#ifndef TINTIN_REPORTER_H
#define TINTIN_REPORTER_H

#include <string>
#include <fstream>


class Tintin_reporter {
    private:
        std::string _logPath;
        std::ofstream _logFile;

        std::string getCurrentTimestamp() const;
        void writeToLog(const std::string& level, const std::string& message);

    public:
        Tintin_reporter();
        Tintin_reporter(const Tintin_reporter& other);
        Tintin_reporter& operator=(const Tintin_reporter& other);
        ~Tintin_reporter();

        Tintin_reporter(const std::string& logPath);

        void info(const std::string& message);
        void log(const std::string& message);
        void error(const std::string& message);

        bool isOpen() const;
        void close();
};

#endif