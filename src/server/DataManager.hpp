#pragma once
#include "Config.hpp"
#include <unordered_map>
#include <pthread.h>
#include <shared_mutex>

namespace storage
{
    extern std::string logger_name;
    // 文件的属性信息
    struct StorageInfo
    {
        std::string storage_path_; // 存储路径
        std::string url_;          // 请求的资源路径
        time_t mtime_;             // 上次修改时间
        time_t atime_;             // 上次访问时间
        size_t size_;              // 文件大小
        // 填充文件属性信息
        bool NewStorageInfo(const std::string &storage_path)
        {
            logsystem::GetLogger(logger_name)->Info("init %s file info", storage_path.c_str());
            storage::FileUtil file(storage_path);
            if (!file.FileExists())
            {
                logsystem::GetLogger(logger_name)->Error("file not exists");
                return false;
            }
            mtime_ = file.GetLastModifiedTime();
            atime_ = file.GetLastAccessTime();
            size_ = file.GetFileSize();

            storage_path_ = storage_path;
            storage::Config *config = storage::Config::GetInstance();
            url_ = config->GetDownloadPrefix();
            logsystem::GetLogger(logger_name)->Info("download_url:%s,mtime_:%s,atime_:%s,fsize_:%d", url_.c_str(), ctime(&mtime_), ctime(&atime_), size_);
            logsystem::GetLogger(logger_name)->Info("NewStorageInfo end");
            return true;
        }

    }; // class StorageInfo

    class DataManager
    {
    public:
        DataManager()
        {
            logsystem::GetLogger(logger_name)->Info("DataManager construc start");
            storage::Config *config = storage::Config::GetInstance();
            storage_file_ = config->GetStorageInfoFile();
            // 加载已存储文件信息

            logsystem::GetLogger(logger_name)->Info("DataManager construc end");
        }

        ~DataManager()
        {
        }

        // 加载已存储文件信息
        bool InitLoad() // 初始化程序运行时从文件读取数据
        {
            logsystem::GetLogger(logger_name)->Info("init datamanager");
            storage::FileUtil f(storage_file_);
            if (!f.FileExists())
            {
                logsystem::GetLogger(logger_name)->Info("there is no storage file info need to load");
                return true;
            }

            std::string body;
            if (!f.GetFileContent(&body))
                return false;

            // 反序列化
            Json::Value root;
            storage::JsonUtil::UnSerialize(body, &root);
            // 3，将反序列化得到的Json::Value中的数据添加到table中
            for (int i = 0; i < root.size(); i++)
            {
                StorageInfo info;
                info.size_ = root[i]["size_"].asInt();
                info.atime_ = root[i]["atime_"].asInt();
                info.mtime_ = root[i]["mtime_"].asInt();
                info.storage_path_ = root[i]["storage_path_"].asString();
                info.url_ = root[i]["url_"].asString();
                Insert(info);
            }
            return true;
        }

        bool Storage()
        { // 每次有信息改变则需要持久化存储一次 ------性能差 可以优化
            /*
            优化方案：
            每次插入都调用 Storage() 落盘：性能瓶颈
            这仍然是你并发吞吐的最大瓶颈：

            Storage() 把整个 table_ 全序列化为 JSON，并同步写入磁盘，每次上传文件都会执行一次。
            并发高时，Storage() 会频繁阻塞，甚至可能被多个线程重复触发，带来严重性能退化。

            1、异步延迟落盘	把落盘任务放入后台线程批处理，比如每 5 秒写一次、或累计改动次数达到阈值再写。
            2、日志式持久化	每次只将更新项写入日志（如 append 到一个 .log 文件），定期合并为全量 JSON 快照。
            3、使用嵌入式数据库（可选）	用 SQLite / LevelDB 代替 FileUtil + JSON 管理结构，省去线程安全 + 序列化问题。
            */
            logsystem::GetLogger(logger_name)->Info("message storage start");
            std::vector<StorageInfo> arr;
            if (!GetAll(&arr))
            {
                logsystem::GetLogger(logger_name)->Warn("GetAll fail,can't get StorageInfo");
                return false;
            }

            Json::Value root; // root中存着json::value对象
            for (auto e : arr)
            {
                Json::Value item;
                item["mtime_"] = (Json::Int64)e.mtime_;
                item["atime_"] = (Json::Int64)e.atime_;
                item["size_"] = (Json::Int64)e.size_;
                item["url_"] = e.url_.c_str();
                item["storage_path_"] = e.storage_path_.c_str();
                root.append(item); // 作为数组
            }

            // 序列化
            std::string body;
            logsystem::GetLogger(logger_name)->Info("new message for StorageInfo:%s", body.c_str());
            JsonUtil::Serialize(root, &body);

            // 写入文件
            FileUtil f(storage_file_);

            if (f.WriteToFile(body.c_str(), body.size()) == false)
                logsystem::GetLogger(logger_name)->Error("SetContent for StorageInfo Error");

            logsystem::GetLogger(logger_name)->Info("message storage end");
            return true;
        }

        bool Insert(const StorageInfo &info)
        {
            logsystem::GetLogger(logger_name)->Info("data_message Insert start");

            {
                std::unique_lock<std::shared_mutex> lock(rwlock_); // 加写锁
                table_[info.url_] = info;
            }

            if (Storage() == false)
            {
                logsystem::GetLogger(logger_name)->Error("data_message Insert:Storage Error");
                return false;
            }
            logsystem::GetLogger(logger_name)->Info("data_message Insert end");
            return true;
        }

        bool Update(const StorageInfo &info)
        {
            logsystem::GetLogger(logger_name)->Info("data_message Update start");
            {
                std::unique_lock<std::shared_mutex> lock(rwlock_); // 加写锁
                table_[info.url_] = info;
            }

            if (Storage() == false)
            {
                logsystem::GetLogger(logger_name)->Error("data_message Update:Storage Error");
                //回滚 table_ 
                std::unique_lock<std::shared_mutex> lock(rwlock_); // 加写锁
                table_.erase(info.url_);
                return false;
            }
            logsystem::GetLogger(logger_name)->Info("data_message Update end");
            return true;
        }
        bool GetOneByURL(const std::string &key, StorageInfo *info)
        {

            std::shared_lock<std::shared_mutex> lock(rwlock_);
            // URL是key，所以直接find()找
            if (table_.find(key) == table_.end())
            {
                return false;
            }
            *info = table_[key]; // 获取url对应的文件存储信息

            return true;
        }
        bool GetOneByStoragePath(const std::string &storage_path, StorageInfo *info)
        { // 有问题
            std::shared_lock<std::shared_mutex> lock(rwlock_);
            // 遍历 通过realpath字段找到对应存储信息
            for (auto e : table_)
            {
                if (e.second.storage_path_ == storage_path)
                {
                    *info = e.second;
                    return true;
                }
            }
            return false;
        }
        bool GetAll(std::vector<StorageInfo> *arry)
        {
            std::shared_lock<std::shared_mutex> lock(rwlock_);

            for (auto e : table_)
                arry->emplace_back(e.second);

            return true;
        }

    private:
        std::string storage_file_;
        std::shared_mutex rwlock_;
        // url-storage_info
        std::unordered_map<std::string, StorageInfo> table_;
    }; // class DataManager

} // namespace stroage