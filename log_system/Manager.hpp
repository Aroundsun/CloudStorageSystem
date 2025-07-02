#pragma once
#include <unordered_map>
#include "AsyncLogger.hpp"

namespace logsystem
{
    // 通过单例对象对日志管理器进行管理 懒汉式单例模式
    class LoggerManager
    {
    public:
        static LoggerManager &GetInstance()
        {
            static LoggerManager instance;
            return instance;
        }
        bool LoggerExist(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            return logger_map_.find(name) != logger_map_.end();
        }

        void AddLogger(AsyncLogger::ptr &&AsyncLogger)
        {
            if (LoggerExist(AsyncLogger->Name()))
                return;
            std::unique_lock<std::mutex> lock(mutex_);
            logger_map_.insert(std::make_pair(AsyncLogger->Name(), std::move(AsyncLogger)));
        }

        AsyncLogger::ptr GetLogger(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (logger_map_.find(name) == logger_map_.end())
            {
                return nullptr; // 如果日志器不存在，返回空指针
            }
            return logger_map_[name];
        }
        /*
            如果用的是 C++17，可以换成 std::scoped_lock

            std::scoped_lock lock(mtx_);
            功能同 unique_lock，但更轻量（无法显式 unlock，作用域即锁域）。

            若修改 mtx_ 为 std::shared_mutex，只读查询可用 std::shared_lock
            这样多个读线程能并发访问，写线程再用 unique_lock。

            在函数前加 [[ndiscard]o]（C++17）

            [[nodiscard]] AsyncLogger::ptr GetLogger(const std::string& name);
            编译器会在调用者忽略返回值时给出警告，防止“拿了单例却没用”。
        */
        AsyncLogger::ptr DefaultLogger()
        {
            return default_logger_;
        }

    private:
        LoggerManager()
        {
            std::unique_ptr<logsystem::LoggerBuilder> builder(new logsystem::LoggerBuilder());
            builder->BuildLoggerName("default");
            default_logger_ = builder->Build();
            logger_map_.insert(std::make_pair("default", default_logger_));
        }

    private:
        std::mutex mutex_;                                                        // 互斥锁，保护日志器的线程安全
        logsystem::AsyncLogger::ptr default_logger_;                              // 默认日志器
        std::unordered_map<std::string, logsystem::AsyncLogger::ptr> logger_map_; // 存储日志器的映射表
    };

} // namespace logsystem