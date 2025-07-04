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
        
        FileUtil(const std::string &filename):filename_(filename)
        {
        }
        //检查文件是否存在
        bool FileExists()const;

        //获取文件大小
        size_t GetFileSize() const;

        //获取上一次文件的访问时间
        time_t GetLastAccessTime() const;

        //获取上一次文件的修改时间
        time_t GetLastModifiedTime() const;

        //获取文件名称
        std::string GetFileName() const;

        //从磁盘文件中 按偏移量 (pos) 读取指定字节数 (len) 到content 中
        bool GetPosLen(std::string* content, size_t pos, size_t len);

        //获取文件的内容
        bool GetFileContent(std::string* content) const;

        // 将内容写入到文件中
        bool WriteToFile(const std::string &content) const;

        //创建文件目录
        bool CreateDirectory() const;

        //扫描某目录下所有 普通文件 的名字并返回
        bool ScanDirectory(std::vector<std::string>) const;

        // 压缩内容到文件中，format 为压缩格式
        bool Compress(const std::string &content, int format);

        // 解压缩文件到指定路径
        bool UnCompress(std::string &download_path);

    private:
        std::string filename_;
    };

    class JsonUtil
    {
    public:
        // 将 Json::Value 序列化为字符串
        static bool Serialize(const Json::Value &val, std::string *str);
        // 将字符串反序列化为 Json::Value
        static bool UnSerialize(const std::string &str, Json::Value *val);
        
    };
} // namespace storage