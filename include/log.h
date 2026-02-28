#define INFO(msg) std::cout << "[INFO] " << msg << std::endl

#define WARN(msg) std::cout << "\033[33m[WARN] " << msg << "\033[0m" << std::endl

#define FATAL_ERROR(msg)                                                                           \
    do                                                                                             \
    {                                                                                              \
        std::cerr << "\033[31m[ERROR] " << msg << "\033[0m" << std::endl;                          \
        std::exit(EXIT_FAILURE);                                                                   \
    } while (0)

#define HIGHLIGHT(msg) std::cout << "\033[1;34m[INFO] " << msg << "\033[0m" << std::endl
