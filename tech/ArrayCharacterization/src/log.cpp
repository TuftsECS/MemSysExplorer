#include "log.hpp"

#include <iostream>
#include <mutex>

static std::mutex logMutex;
static std::ostream* logStream = &std::cerr;

void mse::log::setLogStream(std::ostream& stream) noexcept {
    std::lock_guard lock(logMutex);
    logStream = &stream;
}

void mse::log::write(const std::string& tag, const std::string& message) {
    std::lock_guard lock(logMutex);
    *logStream << tag << message << std::endl;
}
