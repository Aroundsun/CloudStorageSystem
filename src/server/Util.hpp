#pragma once
#include "jsoncpp/json/json.h"
#include <cassert>
#include <sstream>
#include <memory>
#include "bundle.h"
#include <iostream>
#include <experimental/filesystem>
#include <string>
#include <sys/stat.h>
#include <vector>
#include <fstream>
#include "../../log_system/Logger.hpp"


using namespace logsystem;
namespace storage
{
    // 日志名称
    static const std::string logger_name = "asynclogger";
    // 将字符串转换为十六进制
    static unsigned char ToHex(unsigned char x)
    {
        return x > 9 ? x + 55 : x + 48;
    }
    // 将十六进制字符串转换为字符
    static unsigned char FromHex(unsigned char x)
    {
        unsigned char y;
        if (x >= 'A' && x <= 'Z')
            y = x - 'A' + 10;
        else if (x >= 'a' && x <= 'z')
            y = x - 'a' + 10;
        else if (x >= '0' && x <= '9')
            y = x - '0';
        else
            assert(0);
        return y;
    }

    // 把 URL 百分号编码的字符串解码成原始文本
    static std::string UrlDecode(const std::string &str)
    {
        std::string result;
        size_t length = str.length();

        for (size_t i = 0; i < length; ++i)
        {
            if (str[i] == '+')
            {
                result += ' ';
            }
            else if (str[i] == '%' && i + 2 < length)
            {
                unsigned char high = FromHex(str[i + 1]);
                unsigned char low = FromHex(str[i + 2]);
                result += static_cast<char>(high * 16 + low);
                i += 2;
            }
            else
            {
                result += str[i];
            }
        }
        return result;
    }

    // 文件操作工具类
    class FileUtil
    {
    public:
        FileUtil(const std::string &filename) : filename_(filename)
        {
        }
        // 检查文件是否存在
        bool FileExists() const
        {
            return std::filesystem::exists(filename_);
        }

        // 获取文件大小
        size_t GetFileSize() const
        {
            struct stat file_stat;
            if (stat(filename_.c_str(), &file_stat) != 0)
            {
                logsystem::GetLogger(logger_name)->Error("%s, Get file size failed: %s", filename_.c_str(), strerror(errno));
                return 0;
            }

            return file_stat.st_size;
        }

        // 获取上一次文件的访问时间
        time_t GetLastAccessTime() const
        {
            struct stat file_stat;
            if (stat(filename_.c_str(), &file_stat) != 0)
            {
                logsystem::GetLogger(logger_name)->Info("%s, Get file access time failed: %s", filename_.c_str(), strerror(errno));
                return -1;
            }
            return file_stat.st_atime;
        }

        // 获取上一次文件的修改时间
        time_t GetLastModifiedTime() const
        {
            struct stat file_stat;
            if (stat(filename_.c_str(), &file_stat) != 0)
            {
                logsystem::GetLogger(logger_name)->Info("%s, Get file access time failed: %s", filename_.c_str(), strerror(errno));
                return -1;
            }
            return file_stat.st_mtime;
        }

        // 获取文件名称
        std::string GetFileName() const
        {
            auto pos = filename_.find_last_of("/");
            if (pos == std::string::npos)
            {
                return filename_;
            }
            return filename_.substr(pos + 1, std::string::npos);
        }

        // 从磁盘文件中 按偏移量 (pos) 读取指定字节数 (len) 到content 中
        bool GetPosLen(std::string *content, size_t pos, size_t len)
        {
            if (pos < 0 || len <= 0)
            {
                logsystem::GetLogger(logger_name)->Info("GetPosLen: pos or len is invalid: pos=%zu, len=%zu", pos, len);
                return false;
            }
            std::ifstream ifs(filename_, std::ios::binary);
            if (!ifs)
            {
                logsystem::GetLogger(logger_name)->Info("%s,file open error", filename_.c_str());
                return false;
            }
            ifs.seekg(pos, std::ios::beg); // 将文件指针移动到指定位置
            content->resize(len);          // 调整 content 的大小
            ifs.read(&(*content)[0], len);
            if (!ifs.good())
            {
                logsystem::GetLogger(logger_name)->Info("%s,read file content error", filename_.c_str());
                return false;
            }
            return true;
        }

        // 获取文件的内容
        bool GetFileContent(std::string *content)
        {
            return GetPosLen(content, 0, GetFileSize());
        }

        //

        // 创建文件目录
        bool CreateDirectory()
        {
            if (!FileExists() && !std::filesystem::create_directories(filename_))
            {
                logsystem::GetLogger(logger_name)->Error("%s file Create Directory faild : %s,", filename_.c_str(), strerror(errno));
                return false;
            }
            return true;
        }

        // 扫描某目录下所有 普通文件 的名字并返回
        bool ScanDirectory(std::vector<std::string> *dir) const
        {
            for (auto &p : std::filesystem::directory_iterator(filename_))
            {
                if (std::filesystem::is_directory(p))
                    continue;
                dir->push_back(std::filesystem::path(p).relative_path().string());
            }
            return;
        }
        // 将数据写到文件
        bool WriteToFile(const char *content, size_t len)
        {
            std::ofstream ofs(filename_);
            ofs.open(filename_, std::ios::binary);
            if (!ofs)
            {
                logsystem::GetLogger(logger_name)->Error("%s file open faild: %s,",filename_.c_str(),strerror(errno));
                return false;
            }
            ofs.write(content,static_cast<std::streamsize>(len));
            if( !ofs.good())
            {
                logsystem::GetLogger(logger_name)->Error("%s file write faild: %s,",filename_.c_str(),strerror(errno));
                return false;
            }
            return true;
        }
        // 压缩字符串到文件中，format 为压缩格式
        bool Compress(const std::string &content, int format)
        {
            std::string packed = bundle::pack(format, content);
            if (packed.size() == 0)
            {
                logsystem::GetLogger("asynclogger")->Info("Compress packed size error:%d", packed.size());
                return false;
            }
            // 将压缩的数据写入压缩包文件中
            FileUtil f(filename_);
            if (f.WriteToFile(packed.c_str(), packed.size()) == false)
            {
                logsystem::GetLogger(logger_name)->Info("filename:%s, Compress SetContent error", filename_.c_str());
                return false;
            }
            return true;
        }

        // 解压缩文件到指定路径
        bool UnCompress(std::string &download_path)
        {
            //将压缩包的数据取出来
            std::string body;
            if(! this->GetFileContent(&body))
            {
                logsystem::GetLogger(logger_name)->Error("%s GetFileContent faild:%s",filename_.c_str(),strerror(errno));
                return false;
            }
            //解压
            std::string UnCompressBody = bundle::unpack(body);
            //解压后的数据写入download_path
            FileUtil fu(download_path);
            if(fu.WriteToFile(UnCompressBody.c_str(),UnCompressBody.size()))
            {
                logsystem::GetLogger(logger_name)->Error("%s file wirte faild %s",download_path.c_str(),strerror(errno));
                return false;
            }
            return true;
        }

    private:
        std::string filename_;
    };

    class JsonUtil
    {
    public:
        // 将 Json::Value 序列化为字符串
        static bool Serialize(const Json::Value &val, std::string *str)
        {
            Json::StreamWriterBuilder swb;
            std::unique_ptr<Json::StreamWriter> sw_ptr(swb.newStreamWriter());
            std::stringstream ss;
            if(! sw_ptr->write(val,&ss) )
            {
                logsystem::GetLogger(logger_name)->Error("Serialize faild");
                return false;
            }
            *str = ss.str();
            return true;
        }
        // 将字符串反序列化为 Json::Value
        static bool UnSerialize(const std::string &str, Json::Value *val)
        {
            Json::CharReaderBuilder crb;
            std::unique_ptr<Json::CharReader> cr_ptr(crb.newCharReader());
            std::string err;
            if(! cr_ptr->parse(str.c_str(),str.c_str()+str.size(),val,&err))
            {
                logsystem::GetLogger(logger_name)->Error("UnSerialize faild,error:%s",err.c_str());
                return false;
            }
            return true;
        }
    };
} // namespace storage