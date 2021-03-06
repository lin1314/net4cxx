//
// Created by yuwenyong on 17-9-19.
//

#include "net4cxx/common/global/loggers.h"


NS_BEGIN


Logger *gAccessLog = nullptr;
Logger *gAppLog = nullptr;
Logger *gGenLog = nullptr;


void LogUtil::initGlobalLoggers() {
    gAccessLog = Logging::getLogger("access");
    gAppLog = Logging::getLogger("application");
    gGenLog = Logging::getLogger("general");
}

void LogUtil::enablePrettyLogging(const OptionParser *options) {
    const auto &logLevel = options->get<std::string>("log_level");
    if (logLevel == "none") {
        return;
    }
    Logging::init();
    Severity severity = Logging::toSeverity(logLevel);
    if (options->has("log_file_prefix")) {
        const auto &logFilePrefix = options->get<std::string>("log_file_prefix");
        size_t fileMaxSize = options->get<size_t>("log_file_max_size");
        RotatingFileSink sink(logFilePrefix, fileMaxSize);
        sink.setFilter("", severity);
        Logging::addSink(sink);
    }
    if (options->has("log_to_console") || !options->has("log_file_prefix")) {
        ConsoleSink sink;
        sink.setFilter("", severity);
        Logging::addSink(sink);
    }
}

void LogUtil::defineLoggingOptions(OptionParser *options) {
#ifdef NET4CXX_NDEBUG
    options->addArgument<std::string>("log_level", "Set the log level", std::string("info"), {}, "logging");
#else
    options->addArgument<std::string>("log_level", "Set the log level", std::string("debug"), {}, "logging");
#endif
    options->addArgument("log_to_console", "Send log output to stderr", "logging");
    options->addArgument<std::string>("log_file_prefix", "Path prefix for log files", {}, {}, "logging");
    options->addArgument<size_t>("log_file_max_size", "Max size of log files", 100 * 1000 * 1000, {}, "logging");
    options->addParseCallback([options] () {
        enablePrettyLogging(options);
    });
}

NS_END