//
// Created by Lby on 2017/6/6.
//

#ifndef ORZ_UTILS_LOG_H
#define ORZ_UTILS_LOG_H

#include <iostream>
#include <sstream>
#include "format.h"
#include "except.h"
#include <cstring>

#undef NONE
#undef DEBUG
#undef STATUS
#undef INFO
#undef ERROR
#undef FATAL

namespace orz {

    enum LogLevel {
        NONE = 0,
        DEBUG = 1,
        STATUS = 2,
        INFO = 3,
        ERROR = 4,
        FATAL = 5,
    };

    extern LogLevel InnerGlobalLogLevel;

    inline LogLevel GlobalLogLevel(LogLevel level) {
        LogLevel pre_level = InnerGlobalLogLevel;
        InnerGlobalLogLevel = level;
        return pre_level;
    }

    inline LogLevel GlobalLogLevel() {
        return InnerGlobalLogLevel;
    }

    class Log {
    public:
        Log(LogLevel level, std::ostream &log = std::cout)
                : m_level(level), m_log(log) {
        }

        ~Log() {
            flush();
        }

        const std::string message() const {
            return m_buffer.str();
        }

        template<typename T>
        Log &operator()(const T &message) {
            if (m_level >= InnerGlobalLogLevel) {
                m_buffer << message;
            }
            return *this;
        }

        template<typename T>
        Log &operator<<(const T &message) {
            return operator()(message);
        }

        using Method = Log &(Log &);

        Log &operator<<(Method method) {
            if (m_level >= InnerGlobalLogLevel) {
                return method(*this);
            }
            return *this;
        }

        void flush()
        {
            std::string level_str = "Unkown";
            switch (m_level) {
            case NONE: return;
            case DEBUG: level_str = "DEBUG"; break;
            case STATUS: level_str = "STATUS"; break;
            case INFO: level_str = "INFO"; break;
            case ERROR: level_str = "ERROR"; break;
            case FATAL: level_str = "FATAL"; break;
            }
            if (m_level >= InnerGlobalLogLevel) {
                auto msg = m_buffer.str();
                m_buffer.str("");
                m_buffer << level_str << ": " << msg << std::endl;
                m_log << m_buffer.str();
            }
            m_level = NONE;
            m_buffer.str("");
            m_log.flush();
        }

    private:
        LogLevel m_level;
        std::ostringstream m_buffer;
        std::ostream &m_log;

        Log(const Log &other) = delete;
        Log &operator=(const Log&other) = delete;
    };

    inline Log &crash(Log &log)
    {
        const auto msg = log.message();
        log.flush();
        throw Exception(msg);
    }
}

#ifdef ORZ_SOLUTION_DIR
#define ORZ_LOCAL_FILE ( \
    std::strlen(ORZ_SOLUTION_DIR) + 1 < std::strlen(__FILE__) \
    ? ((const char *)(__FILE__ + std::strlen(ORZ_SOLUTION_DIR) + 1)) \
    : ((const char *)(__FILE__)) \
    )
#else
#define ORZ_LOCAL_FILE (orz::Split(__FILE__, R"(/\)").back())
#endif

#define ORZ_LOG(level) (orz::Log(level))("[")(ORZ_LOCAL_FILE)(":")(__LINE__)("]: ")
#define ORZ_TIME(level) (orz::Log(level))("[")(orz::now_time())("]: ")
#define ORZ_TIME_LOG(level) (orz::Log(level))("[")(orz::now_time())("][")(ORZ_LOCAL_FILE)(":")(__LINE__)("]: ")

#endif //ORZ_UTILS_LOG_H
