#pragma once
#include <cassert>
#include <fstream>
#include <memory>
#include <unistd.h>
#include <fcntl.h>  // open
#include <unistd.h> // fsync, close

#include "Util.hpp"

extern logsystem::Config *config;

namespace logsystem
{
    // 日志刷新接口类
    class LogFlush
    {
    public:
        using ptr = std::shared_ptr<LogFlush>;
        virtual ~LogFlush() {};
        virtual void Flush(const char *data, size_t len) = 0;
    };

    // 标准输出日志刷新类
    class StdoutFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<StdoutFlush>;
        void Flush(const char *data, size_t len) override
        {
            assert(data != nullptr && len > 0);
            std::cout.write(data, len); // 将数据写入标准输出
            std::cout.flush();          // 刷新输出流
            if (config->flush_log == 1) // 如果配置要求使用 fflush 刷新
            {
                fflush(stdout); // 把用户缓冲写入操作系统缓存
            }
            else if (config->flush_log == 2) // 如果配置要求使用 fsync 刷新
            {
                int fd = fileno(stdout); // 获取标准输出的文件描述符
                if (fd >= 0)
                {
                    fflush(stdout); // 把用户缓冲写入操作系统缓存
                    fsync(fd);      // 刷新文件描述符到磁盘
                }
                else
                {
                    std::cerr << "StdoutFlush: 获取标准输出文件描述符失败" << std::endl;
                }
            }
            else
            {
                std::cerr << "StdoutFlush: 无效的刷新配置" << std::endl;
            }
        }
    };
    // 文件日志刷新类
    class FileFlush : public LogFlush
    {
    public:
        using ptr = std::shared_ptr<FileFlush>;
        FileFlush(const std::string filename)
            : filename_(filename)
        {

            try
            {
                // 创建所给目录
                logsystem::File::CreateDirectory(filename);
            }
            catch (const std::invalid_argument &e)
            {
                std::cerr << "空目录：" << e.what() << std::endl;
            }
            ofs_.open(filename, std::ios::app | std::ios::binary);
            if (!ofs_)
            {
                std::cout << __FILE__ << __LINE__ << "open log file failed" << std::endl;
                perror(NULL);
            }
        }
        void Flush(const char *data, size_t len) override
        {
            // 写入
            ofs_.write(data, static_cast<std::streamsize>(len));
            // 检查写错误
            if (!ofs_)
            {
                std::cerr << __FILE__ << ":" << __LINE__ << "  write log file failed" << std::endl;
                throw std::runtime_error("ofstream write fail");
            }
            if (config->flush_log == 1)
            {
                ofs_.flush(); // 刷新输出流
            }
            else if (config->flush_log == 2)
            {
                ofs_.flush();                                 // ① 用户缓冲 → 内核
                int fd = ::open(filename_.c_str(), O_RDONLY); // ② 重新拿 fd
                if (fd >= 0)
                {
                    ::fsync(fd); // ③ 内核 → 硬盘
                    ::close(fd);
                }
            }
            else
            {
                perror("open for fsync failed");
            }
        }

    private:
        std::ofstream ofs_;
        std::string filename_;
    };

    // 滚动文件日志刷新类
    class RollingFileFlush : public LogFlush
    {
    };

    // 工厂类用于创建不同类型的日志刷新器
    class LogFlushFactory
    {
    };

} // namespace logsystem
