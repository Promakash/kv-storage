#include <cstring>
#include <fstream>
#include <sstream>

#include <sys/socket.h>

#include <dirent.h>

namespace Utils {

size_t GetFDCount(pid_t pid) {
    std::stringstream ss;
    ss << "/proc/" << pid << "/fd";

    std::string path = ss.str();
    size_t count = 0;

    DIR* dir = opendir(path.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                count++;
            }
        }
        closedir(dir);
    }

    return count;
}

bool IsPortListening(uint16_t port) {
    std::string command = "netstat -tuln";
    std::string result;
    char buffer[128];

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error executing command. Try to install netstat: sudo apt install net-tools " << std::endl;
        return false;
    }

    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }

    pclose(pipe);

    std::istringstream iss(result);
    std::string line;
    while (std::getline(iss, line)) {
        std::istringstream issLine(line);
        std::string protocol, recv_q, send_q, localAddress, foreignAddress, state;
        int localPort;

        issLine >> protocol >> recv_q >> send_q >> localAddress >> foreignAddress >> state;

        size_t colonPos = localAddress.rfind(':');
        if (colonPos != std::string::npos) {
            std::string portStr = localAddress.substr(colonPos + 1);
            localPort = std::stoi(portStr);
            
            if (localPort == port) {
                return true;
            }
        }
    }

    return false;
}


class FDBalanceChecker {
public:
    FDBalanceChecker(): fd_count_before_scope(GetFDCount(getpid()))  {
    }

    ~FDBalanceChecker() {
        int fd_count_after_scope = GetFDCount(getpid());
        assert(fd_count_before_scope - fd_count_after_scope == 0);
    }

private:
    int fd_count_before_scope;
};

} // namespace Utils end