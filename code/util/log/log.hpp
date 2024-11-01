#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <list>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <functional>
#include <time.h>
#include <cstdarg>
#include <pthread.h>
#include <cstdio>
#include <mutex>
#include "BlockQueue.hpp"

namespace MindbniM
{
    class Logger;
    // 日志事件
    class LogEvent
    {
    public:
        using ptr = std::shared_ptr<LogEvent>;
        LogEvent(std::shared_ptr<Logger> logger, const std::string &file, int line,
                 uint32_t threadId, uint32_t fiberId, uint64_t time, const char *str, ...);
        LogEvent(std::shared_ptr<Logger> logger, const std::string &file, int line,
                 uint32_t threadId, uint32_t fiberId, uint64_t time, const std::string &message);
        LogEvent() {};

        std::string _file;               // 文件名
        int _line;                       // 行号
        uint32_t _threadId = 0;          // 线程id
        uint32_t _fiberId = 0;           // 协程id
        uint64_t _time;                  // 时间戳
        std::string _message;            // 消息
        std::shared_ptr<Logger> _logger; // 所属日志器

        static const int s_mess_maxlen; // 一条日志信息最大字符数
    };
    const int LogEvent::s_mess_maxlen = 1024;
    // 日志等级
    class LogLevel
    {
    public:
        enum Level
        {
            UNKNOW = 0,
            DEBUG = 1,
            INFO = 2,
            WARNING = 3,
            ERROR = 4,
            FATAL = 5
        };
        // 输出对应的字符串
        static std::string to_string(LogLevel::Level level)
        {
            switch (level)
            {
#define XX(name)      \
    case Level::name: \
        return #name; \
        break;
                XX(DEBUG)
                XX(INFO)
                XX(WARNING)
                XX(ERROR)
                XX(FATAL)
            default:
                return "UNKNOW";
#undef XX
            }
            return "UNKNOW";
        }
    };
    // 日志格式器

    // 格式："%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
    //*  %m 消息
    //*  %p 日志级别
    //*  %c 日志名称
    //*  %t 线程id
    //*  %n 换行
    //*  %d 时间
    //*  %f 文件名
    //*  %l 行号
    //*  %T 制表符
    //*  %F 协程id
    const std::string DEFAULT_FORMAT = "[%p][%d{%Y-%m-%d %H:%M:%S}][%f : %l]%m%n";
    class LogFormatter
    {
    public:
        using ptr = std::shared_ptr<LogFormatter>;
        // 对给出的日志格式初始化m_items
        LogFormatter(const std::string &formatstr = DEFAULT_FORMAT) : m_format(formatstr) { init(); }
        // 上层总解析
        std::string format(LogLevel::Level level, LogEvent::ptr event);
        void init();
        // 日志解析方法
        class FormatItem
        {
        public:
            using ptr = std::shared_ptr<FormatItem>;
            virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) = 0;
            virtual ~FormatItem() {}
        };

    private:
        std::string m_format;                 // 格式
        std::vector<FormatItem::ptr> m_items; // 日志格式解析后需要的方法
        bool m_error = false;                 // 日志格式是否错误
    };
    // 日志输出地
    class LogAppend
    {
    public:
        using ptr = std::shared_ptr<LogAppend>;
        LogAppend(LogLevel::Level level = LogLevel::Level::DEBUG, const std::string &format = DEFAULT_FORMAT)
            : m_level(level), m_format(std::make_shared<LogFormatter>(format))
        {
        }
        virtual ~LogAppend() {}
        virtual void log(LogLevel::Level level, LogEvent::ptr event) = 0;

    protected:
        LogLevel::Level m_level;
        LogFormatter::ptr m_format;
    };
    // 日志器
    class Logger
    {
    public:
        using ptr = std::shared_ptr<Logger>;
        Logger(const std::string &name = "root");
        void log(LogLevel::Level level, LogEvent::ptr event);
        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warning(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);
        void addAppend(LogAppend::ptr append);
        void delAppend(LogAppend::ptr append);
        void set_level(LogLevel::Level level) { m_level = level; }
        LogLevel::Level get_level() const { return m_level; }
        std::string get_name() { return m_name; }

    private:
        std::string m_name;
        LogLevel::Level m_level = LogLevel::Level::DEBUG;
        std::list<LogAppend::ptr> m_appends;
    };
    // 日志管理器
    class LoggerManager
    {
    public:
        using ptr = std::shared_ptr<LoggerManager>;
        static LoggerManager *GetInstance()
        {
            static LoggerManager LogMa;
            return &LogMa;
        }
        Logger::ptr get_root() { return m_root; }
        Logger::ptr get_logger(const std::string &name)
        {
            m_mutex.lock();
            auto it = m_loggers.find(name);
            if (it == m_loggers.end())
            {
                m_loggers[name] = std::make_shared<Logger>(name);
                m_mutex.unlock();
                return m_loggers[name];
            }
            else
            {
                m_mutex.unlock();
                return m_loggers[name];
            }
        }

    private:
        LoggerManager() { m_root = std::make_shared<Logger>("root"); }
        LoggerManager(const LoggerManager &) = delete;
        void operator=(const LoggerManager &) = delete;
        Logger::ptr m_root;
        std::map<std::string, Logger::ptr> m_loggers;
        std::mutex m_mutex;
    };
    class Stdout_LogAppend : public LogAppend
    {
    public:
        using ptr = std::shared_ptr<Stdout_LogAppend>;
        Stdout_LogAppend(LogLevel::Level level = LogLevel::Level::DEBUG, const std::string &format = DEFAULT_FORMAT) : LogAppend(level, format) {}
        virtual void log(LogLevel::Level levle, LogEvent::ptr event) override;

    private:
        std::mutex m_mutex;
    };
    class Fileout_LogAppend : public LogAppend
    {
    public:
        using ptr = std::shared_ptr<Fileout_LogAppend>;
        Fileout_LogAppend(const std::string &filename, LogLevel::Level level = LogLevel::Level::DEBUG, const std::string &format = DEFAULT_FORMAT);
        virtual void log(LogLevel::Level level, LogEvent::ptr evnet) override;
        bool reopen();

    private:
        std::string m_filename;
        std::ofstream m_file;
        std::mutex m_mutex;
    };
    class Asyn_Stdout_LogAppend : public LogAppend
    {
    public:
        using ptr = std::shared_ptr<Asyn_Stdout_LogAppend>;
        Asyn_Stdout_LogAppend(LogLevel::Level level = LogLevel::Level::DEBUG, const std::string &format = DEFAULT_FORMAT) : LogAppend(level, format)
        {
        }
        virtual void log(LogLevel::Level level, LogEvent::ptr event) override;
    };
    class Asyn_Fileout_LogAppend : public LogAppend
    {
    public:
        using ptr = std::shared_ptr<Fileout_LogAppend>;
        Asyn_Fileout_LogAppend(const std::string &filename, LogLevel::Level level = LogLevel::Level::DEBUG, const std::string &format = DEFAULT_FORMAT);
        virtual void log(LogLevel::Level level, LogEvent::ptr evnet) override;
        bool reopen();

    private:
        std::string m_filename;
        std::ofstream m_file;
        std::mutex m_mutex;
    };
    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str = "") {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->_message;
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str = "") {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << LogLevel::to_string(level);
        }
    };
    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &str = "") {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->_line;
        }
    };
    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FilenameFormatItem(const std::string &str = "") {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->_file;
        }
    };
    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str = "") {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->_threadId;
        }
    };
    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str = "") {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->_fiberId;
        }
    };
    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string &str = "") {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->_logger->get_name();
        }
    };
    class DateFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateFormatItem(const std::string format = "%Y-%m-%d %H:%M:%S") : m_format(format) {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            struct tm tm;
            time_t time = event->_time;
            localtime_r(&time, &tm);
            std::string str;
            for (int i = 0; i < m_format.size(); ++i)
            {
                if (m_format[i] != '%')
                    str.push_back(m_format[i]);
                else if (i + 1 < m_format.size())
                {
                    switch (m_format[i + 1])
                    {
                    case 'Y':
                        str += std::to_string(tm.tm_year + 1900);
                        break;
                    case 'm':
                        str += (tm.tm_mon + 1 < 10 ? "0" : "") + std::to_string(tm.tm_mon + 1);
                        break;
                    case 'd':
                        str += (tm.tm_mday < 10 ? "0" : "") + std::to_string(tm.tm_mday);
                        break;
                    case 'H':
                        str += (tm.tm_hour < 10 ? "0" : "") + std::to_string(tm.tm_hour);
                        break;
                    case 'M':
                        str += (tm.tm_min < 10 ? "0" : "") + std::to_string(tm.tm_min);
                        break;
                    case 'S':
                        str += (tm.tm_sec < 10 ? "0" : "") + std::to_string(tm.tm_sec);
                        break;
                    default:
                        str.push_back('%');
                        str.push_back(m_format[i + 1]);
                        break;
                    }
                    ++i;
                }
            }
            os << str;
        }

    private:
        std::string m_format;
    };
    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &str = "") {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << std::endl;
        }

    private:
    };
    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &str = "") {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << "\t";
        }

    private:
    };
    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &str = "") : m_str(str) {}
        virtual void format(std::ostringstream &os, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << m_str;
        }

    private:
        std::string m_str;
    };

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, const std::string &file, int line,
                       uint32_t threadId, uint32_t fiberId, uint64_t time, const std::string &message) : _logger(logger), _file(file), _line(line), _threadId(threadId), _fiberId(fiberId), _time(time), _message(message)
    {
    }
    LogEvent::LogEvent(std::shared_ptr<Logger> logger, const std::string &file, int line,
                       uint32_t threadId, uint32_t fiberId, uint64_t time, const char *str, ...) : _logger(logger), _file(file), _line(line), _threadId(threadId), _fiberId(fiberId), _time(time)
    {
        va_list va;
        va_start(va, str);
        char buff[s_mess_maxlen] = {0};
        vsnprintf(buff, s_mess_maxlen, str, va);
        _message = buff;
        va_end(va);
    }
    Logger::Logger(const std::string &name) : m_name(name)
    {
    }
    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            for (auto &append : m_appends)
            {
                append->log(level, event);
            }
        }
    }
    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::Level::DEBUG, event);
    }
    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::Level::INFO, event);
    }
    void Logger::warning(LogEvent::ptr event)
    {
        log(LogLevel::Level::WARNING, event);
    }
    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::Level::ERROR, event);
    }
    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::Level::FATAL, event);
    }
    void Logger::addAppend(LogAppend::ptr append)
    {
        m_appends.emplace_back(append);
    }
    void Logger::delAppend(LogAppend::ptr append)
    {
        auto it = m_appends.begin();
        while (it != m_appends.end())
        {
            if (*it == append)
            {
                it = m_appends.erase(it);
            }
        }
    }
    void Stdout_LogAppend::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            std::string str = m_format->format(level, event);
            m_mutex.lock();
            std::cout << str;
            m_mutex.unlock();
        }
    }
    Fileout_LogAppend::Fileout_LogAppend(const std::string &filename, LogLevel::Level level, const std::string &format) : m_filename(filename), LogAppend(level, format)
    {
        m_file.open(filename);
    }
    void Fileout_LogAppend::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            std::string str = m_format->format(level, event);
            m_mutex.lock();
            m_file << str;
            m_mutex.unlock();
        }
    }
    bool Fileout_LogAppend::reopen()
    {
        if (m_file.is_open())
        {
            m_file.close();
        }
        m_file.open(m_filename);
        return m_file.is_open();
    }
    Asyn_Fileout_LogAppend::Asyn_Fileout_LogAppend(const std::string &filename, LogLevel::Level level, const std::string &format) : m_filename(filename), LogAppend(level, format)
    {
        m_file.open(filename);
    }
    void Asyn_Fileout_LogAppend::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            std::string str = m_format->format(level, event);
            blockqueue<task>::GetInstance()->push([str, this]
                                                  {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_file<<str; });
        }
    }
    bool Asyn_Fileout_LogAppend::reopen()
    {
        if (m_file.is_open())
        {
            m_file.close();
        }
        m_file.open(m_filename);
        return m_file.is_open();
    }
    void Asyn_Stdout_LogAppend::log(LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            std::string str = m_format->format(level, event);
            blockqueue<task>::GetInstance()->push([=]
                                                  { std::cout << str; });
        }
    }
    std::string LogFormatter::format(LogLevel::Level level, LogEvent::ptr event)
    {
        std::ostringstream os;
        for (auto &fmat : m_items)
        {
            fmat->format(os, level, event);
        }
        return os.str();
    }
    // 默认格式 : "[%p][%d{%Y-%m-%d %H:%M:%S}][%f : %l]%m%n"
    // 可选格式:
    //*  %m 消息
    //*  %p 日志级别
    //*  %c 日志名称
    //*  %t 线程id
    //*  %n 换行
    //*  %d 时间
    //*  %f 文件名
    //*  %l 行号
    //*  %T 制表符
    //*  %F 协程id
    void LogFormatter::init()
    {
        std::vector<std::tuple<std::string, std::string, int>> vec;
        std::string nstr;
        for (size_t i = 0; i < m_format.size(); ++i)
        {
            if (m_format[i] != '%')
            {
                nstr.append(1, m_format[i]);
                continue;
            }

            if ((i + 1) < m_format.size())
            {
                if (m_format[i + 1] == '%')
                {
                    nstr.append(1, '%');
                    continue;
                }

                size_t n = i + 1;
                int fmt_status = 0;
                size_t fmt_begin = 0;

                std::string str;
                std::string fmt;

                while (n < m_format.size())
                {
                    if (!fmt_status && (!isalpha(m_format[n]) && m_format[n] != '{' && m_format[n] != '}'))
                    {
                        str = m_format.substr(i + 1, n - i - 1);
                        break;
                    }
                    if (fmt_status == 0)
                    { // 开始解析时间格式
                        if (m_format[n] == '{')
                        {
                            str = m_format.substr(i + 1, n - i - 1); // str = "d"
                            fmt_status = 1;
                            fmt_begin = n;
                            ++n;
                            continue;
                        }
                    }
                    else if (fmt_status == 1)
                    { // 结束解析时间格式
                        if (m_format[n] == '}')
                        {
                            // fmt = %Y-%m-%d %H:%M:%S
                            fmt = m_format.substr(fmt_begin + 1, n - fmt_begin - 1);
                            fmt_status = 0;
                            ++n;
                            break;
                        }
                    }
                    ++n;
                    if (n == m_format.size())
                    { // 最后一个字符
                        if (str.empty())
                        {
                            str = m_format.substr(i + 1);
                        }
                    }
                }
                if (fmt_status == 0)
                {
                    if (!nstr.empty())
                    {
                        vec.push_back(std::make_tuple(nstr, std::string(), 0)); // 将[ ]放入， type为0
                        nstr.clear();
                    }
                    vec.push_back(std::make_tuple(str, fmt, 1)); //(e.g.) ("d", %Y-%m-%d %H:%M:%S, 1) type为1
                    i = n - 1;                                   // 跳过已解析的字符，让i指向当前处理的字符，下个for循环会++i处理下个字符
                }
                else if (fmt_status == 1)
                {
                    std::cout << "Pattern parde error: " << m_format << " - " << m_format.substr(i) << std::endl;
                    m_error = true;
                    vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
                }
            }
        }

        if (!nstr.empty())
        {
            vec.push_back(std::make_tuple(nstr, "", 0)); //(e.g.) 最后一个字符为[ ] :
        }

        static std::map<std::string, std::function<FormatItem::ptr(const std::string &fmt)>> s_format_items = {
#define XX(str, C) \
    {#str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); }}

            XX(m, MessageFormatItem),  // m:消息
            XX(p, LevelFormatItem),    // p:日志级别
            XX(c, NameFormatItem),     // c:日志名称
            XX(t, ThreadIdFormatItem), // t:线程id
            XX(n, NewLineFormatItem),  // n:换行
            XX(d, DateFormatItem),     // d:时间
            XX(f, FilenameFormatItem), // f:文件名
            XX(l, LineFormatItem),     // l:行号
            XX(T, TabFormatItem),      // T:Tab
            XX(F, FiberIdFormatItem),  // F:协程id

#undef XX
        };

        for (auto &i : vec)
        {
            if (std::get<2>(i) == 0)
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                auto it = s_format_items.find(std::get<0>(i));
                if (it == s_format_items.end())
                {
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                    m_error = true;
                }
                else
                {
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }
            // std::cout<<std::get<0>(i)<<" "<<std::get<1>(i)<<" "<<std::get<2>(i)<<std::endl;
        }
    }
#define LOG_ROOT() LoggerManager::GetInstance()->get_root()
#define LOG_NAME(name) LoggerManager::GetInstance()->get_logger(name)

#define LOG_EVENT(logger, str, ...) std::make_shared<LogEvent>(logger, __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__)

#define STDOUT_APPEND(level, format) std::make_shared<Stdout_LogAppend>(level, format)
#define STDOUT_APPEND_DEFAULT(level) std::make_shared<Stdout_LogAppend>()

#define LOG_ROOT_ADD_STDOUT_APPEND_DEFAULT() LOG_ROOT()->addAppend(STDOUT_APPEND_DEFAULT())
#define LOG_ROOT_ADD_STDOUT_APPEND(level, format) LOG_ROOT()->addAppend(STDOUT_APPEND(level, format))
#define FILEOUT_APPEND(filename, level, format) std::make_shared_<Fileout_LogAppend>(filename, level, format)
#define LOG_ROOT_ADD_FILEOUT_APPEND(filename, level, format) LOG_ROOT()->addAppend(FILEOUT_APPEND(filename, level, format))
#define LOG_ADD_STDOUT_APPEND_DEFAULT(name) LOG_NAME(name)->addAppend(STDOUT_APPEND_DEFAULT())
#define LOG_ADD_STDOUT_APPEND(name, level, format) LOG_NAME(name)->addAppend(STDOUT_APPEND(level, format))
#define CXX_OUT
#ifdef C_OUT
#define LOG_ROOT_DEBUG(str, ...) LOG_ROOT()->debug(std::make_shared<LogEvent>(LOG_ROOT(), __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__))
#define LOG_ROOT_INFO(str, ...) LOG_ROOT()->info(std::make_shared<LogEvent>(LOG_ROOT(), __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__))
#define LOG_ROOT_WARNING(str, ...) LOG_ROOT()->warning(std::make_shared<LogEvent>(LOG_ROOT(), __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__))
#define LOG_ROOT_ERROR(str, ...) LOG_ROOT()->error(std::make_shared<LogEvent>(LOG_ROOT(), __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__))
#define LOG_ROOT_FATAL(str, ...) LOG_ROOT()->fatal(std::make_shared<LogEvent>(LOG_ROOT(), __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__))

#define LOG_DEBUG(name, str, ...) LOG_NAME(name)->debug(std::make_shared<LogEvent>(LOG_NAME(name), __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__))
#define LOG_INFO(name, str, ...) LOG_NAME(name)->info(std::make_shared<LogEvent>(LOG_NAME(name), __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__))
#define LOG_WARNING(name, str, ...) LOG_NAME(name)->warning(std::make_shared<LogEvent>(LOG_NAME(name), __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__))
#define LOG_ERROR(name, str, ...) LOG_NAME(name)->error(std::make_shared<LogEvent>(LOG_NAME(name), __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__))
#define LOG_FATAL(name, str, ...) LOG_NAME(name)->fatal(std::make_shared<LogEvent>(LOG_NAME(name), __FILE__, __LINE__, pthread_self(), 0, ::time(nullptr), str, ##__VA_ARGS__))
#endif
    class LogOut
    {
    public:
        LogOut(Logger::ptr log,LogLevel::Level l,const char* f,int li):_root(log),_level(l),_file(f),line(li)
        {}
        std::ostringstream& Out(){return _os;}
        ~LogOut()
        {
            _root->log(_level,std::make_shared<LogEvent>(_root,_file,line,pthread_self(),0,::time(nullptr),_os.str()));
        }
    private:
        std::ostringstream _os;
        LogLevel::Level _level;
        Logger::ptr _root;
        const char* _file;
        int line;
    };
#ifdef CXX_OUT
#define LOG_DEBUG(name) LogOut(LOG_NAME(name),LogLevel::Level::DEBUG,__FILE__,__LINE__).Out()
#define LOG_INFO(name) LogOut(LOG_NAME(name),LogLevel::Level::INFO,__FILE__,__LINE__).Out()
#define LOG_WARNING(name) LogOut(LOG_NAME(name),LogLevel::Level::WARNING,__FILE__,__LINE__).Out()
#define LOG_ERROR(name) LogOut(LOG_NAME(name),LogLevel::Level::ERROR,__FILE__,__LINE__).Out()
#define LOG_FATAL(name) LogOut(LOG_NAME(name),LogLevel::Level::FATAL,__FILE__,__LINE__).Out()

#define LOG_ROOT_DEBUG LogOut(LOG_ROOT(),LogLevel::Level::DEBUG,__FILE__,__LINE__).Out()
#define LOG_ROOT_INFO LogOut(LOG_ROOT(),LogLevel::Level::INFO,__FILE__,__LINE__).Out()
#define LOG_ROOT_WARNING LogOut(LOG_ROOT(),LogLevel::Level::WARNING,__FILE__,__LINE__).Out()
#define LOG_ROOT_ERROR LogOut(LOG_ROOT(),LogLevel::Level::ERROR,__FILE__,__LINE__).Out()
#define LOG_ROOT_FATAL LogOut(LOG_ROOT(),LogLevel::Level::FATAL,__FILE__,__LINE__).Out()
#endif

}
