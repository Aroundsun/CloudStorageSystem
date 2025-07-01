#pragma once
#include <atomic>
#include <cassert>
#include <cstdarg>
#include <memory>
#include <mutex>

#include "LogLevel.hpp"
#include "AsyncWorker.hpp"
#include "Message.hpp"
#include "LogFlush.hpp"
#include "CliBackupLog.hpp"
#include "Threadpool.hpp"

namespace logsystem
{

    // 异步日志记录器
    class AsyncLogger
    {
    public:
        using ptr = std::shared_ptr<AsyncLogger>;
        AsyncLogger(std::string logger_name, std::vector<logsystem::LogFlush::ptr> &flush_list,
                    logsystem::AsyncType async_type = logsystem::AsyncType::BLOCKING_BOUNDED)
            : logger_name_(std::move(logger_name)),
              async_worker_(logsystem::AsyncWorker(std::bind(AsyncLogger::RealFlush,this,std::placeholders::_1),async_type)) 
        {}
        ~AsyncLogger(){};
        
        void Debug(const std::string &file,size_t line,const std::string format, ...)
        {

        }
        void Info(const std::string &file,size_t line,const std::string format, ...)
        {
            
        }
        void Warn(const std::string &file,size_t line,const std::string format, ...)
        {
            
        }
        void Error(const std::string &file,size_t line,const std::string format, ...)
        {
            
        }
        void Fatal(const std::string &file,size_t line,const std::string format, ...)
        {
            
        }

    private:
        //将日志信息组织起来并写入文件
        void serialize(logsystem::LogLevel::value level,const std::string &file,size_t line,char* ret)
        {

        }
        //将日志信息写入异步工作器的缓冲区
        void Flush(const char* data,size_t len)
        {

        }
        //异步工作器的回调函数
        void RealFlush(logsystem::Buffer &buffer)
        {

        }
    private:
        std::mutex mutex_; //互斥锁 保护日志器的线程安全
        std::string logger_name_; // 日志器名称
        std::vector<logsystem::LogFlush> flush_list_; //存放各种日志输出方向
        logsystem::AsyncWorker async_worker_; // 异步工作器，负责日志的异步写入
        

    };
    // 异步日志构建器
    // 用于构建异步日志记录器的配置
    class AsyncBulider
    {

    };
} // namespace logsystem