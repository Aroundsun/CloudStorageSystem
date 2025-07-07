#pragma once
#include "DataManager.hpp"

#include <sys/queue.h>
#include <event.h>
// for http
#include <evhttp.h>
#include <event2/http.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <regex>

#include "base64.h" // 来自 cpp-base64 库

extern storage::DataManager* data_;

namespace storage
{
    class Service
    {
    public:
        Service()
        {
            server_port_ = storage::Config::GetInstance()->GetServerPort();
            server_ip_ = storage::Config::GetInstance()->GetServerIp();
            download_prefix_ = storage::Config::GetInstance()->GetDownloadPrefix();
            RegisterRoutes();
        }
        bool RunModule()
        {
            // 初始化环境
            std::unique_ptr<event_base, decltype(&event_base_free)> base(event_base_new(), &event_base_free);
            if (!base)
            {
                // 初始化失败
                return false;
            }
            // 设置ip 端口和地址
            sockaddr_in sin;
            memset(&sin, 0, sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_port = server_port_;
            // 创建Http 服务器
            std::unique_ptr<evhttp, decltype(&evhttp_free)> httpd(evhttp_new(base.get()), &evhttp_free);
            // 绑定http 服务器端口和地址
            if (0 != evhttp_bind_socket(httpd.get(), server_ip_.c_str(), server_port_))
            {
                // 绑定失败
                return false;
            }
            // 设设置回调函数
            evhttp_set_gencb(httpd.get(), GenHandler, NULL);
            // 启动事件循环
            int res = event_base_dispatch(base.get());
            if (-1 == res)
            {
                // 启动失败
                return false;
            }
            else if (1 == res)
            {
                // 循环被明确唤醒但队列已空
            }
            return true;
        }

    private:
        static void GenHandler(struct evhttp_request *req, void *arg)
        {
            const evhttp_uri *uri = evhttp_request_get_evhttp_uri(req);
            std::string path = evhttp_uri_get_path(uri);

            auto it = exact_routes.find(path);
            if (it != exact_routes.end())
            {
                it->second(req, arg);
            }
            
            for (const auto &[prefix, handler] : prefix_routes)
            {
                if (path.rfind(prefix, 0) == 0) // 以 prefix 开头
                {
                    handler(req, arg);
                    return;
                }
            }
            evhttp_send_reply(req, HTTP_NOTFOUND, "Not Found", NULL);
        }

        static void Download(struct evhttp_request *req, void *arg)
        {
            const evhttp_uri *uri = evhttp_request_get_evhttp_uri(req);
            std::string path = evhttp_uri_get_path(uri);
            //解码URL 编吗
            path = storage::UrlDecode(path);
            storage::StorageInfo info;
            //获取请求的文件信息
            if(!data_->GetOneByURL(path,&info))
            {
                logsystem::GetLogger(logger_name)->Error("%s:%d get file faild",evhttp_uri_get_host(uri),evhttp_uri_get_port(uri));
                return;
            }
            
            //如果是压缩文件，则解压

            /*
                用户请求 info.storage_path_ 文件 
                if(用户请求的文件在压缩路径下)
                : 解压这个用户请求的文件
                    到 /tmp/ 下
            */
            //获取下载路径
            std::string download_path = info.storage_path_;
            //如果下载路径不在 原始文件文件路径，则需要解压
            if(info.storage_path_.find(storage::Config::GetInstance()->GetLowStorageDir()) == std::string::npos)
            {
                //download_path文件需要解压
                storage::FileUtil req_filepath(info.storage_path_);

                //需要下载得文件名称
                std::string req_filename(download_path.begin()+download_path.find_last_of("/")+1,download_path.end());
                //解压以后输出位置
                download_path = storage::Config::GetInstance()->GetLowStorageDir() + req_filename;

                //确保中间文件夹存在
                storage::FileUtil dirCreat(storage::Config::GetInstance()->GetLowStorageDir());
                dirCreat.CreateDirectory();

                //将压缩文件解压到 download_path
                req_filepath.UnCompress(download_path);
            }
            
            storage::FileUtil fu(download_path);
            if(!fu.FileExists() )
            { 
                //如果请求的是压缩文件（不是普通文件）  压缩失败了
                if(info.storage_path_.find(storage::Config::GetInstance()->GetDeepStorageDir()) == std::string::npos)
                {

                    return;
                }
                else if(info.storage_path_.find(storage::Config::GetInstance()->GetLowStorageDir()) == std::string::npos)
                {//如果请求的是普通文件（不是压缩文件）， 客户端传的文件不存在
                    
                    return;
                }
            }

            //download_path 用户请求的有效的普通文件
            //开始下载业务，将文件给用户返回
            
            //////////确认文件是否需要断电重传////////////
            bool retrans = false;
            std::string old_etag; 
            //获取文件当前的标签
            std::string if_range = evhttp_find_header(req->input_headers,"If_Range");
            //请求里有 If_range 字段
            if(!if_range.empty())
            {
                old_etag = if_range;
                //旧标签和当前的标签一样, 则断点重传
                if(old_etag == GetETag(info))
                {
                    retrans = true;
                }
            }
            ///////////读取数据文件//////////
            if(!fu.FileExists())
            {
                //记录日志

                //发送错误
                evhttp_send_error(req,404,"No Found");
                return;
            }
            //获取响应的缓冲区
            evbuffer* body = evhttp_request_get_output_buffer(req);

            int fd = open(download_path.c_str(),O_RDONLY);
            if(-1 == fd)
            {
                //文件打开失败，记录日志

                //发送错误
                evhttp_send_error(req,HTTP_INTERNAL,strerror(errno));
                return;
            }
            //将数据从文件复制到 evbuffer 中
            if(-1 == evbuffer_add_file(body,fd,0,fu.GetFileSize()))
            {
                //复制失败，记录日志
                
            }
            ////////设置响应头部字段：ETag，Accept-Ranges: bytes/////////
            evhttp_add_header(req->output_headers,"Accept-Rangle","bytes");
            evhttp_add_header(req->output_headers,"eTag",GetETag(info).c_str());
            evhttp_add_header(req->output_headers,"Content-Type", "application/octet-stream");

            if(!retrans)
            {
                evhttp_send_reply(req, HTTP_OK, "Success", NULL);
                //发送完成，记录日志

            }
            else
            {
                //假重传，后边在扩充断点重传功能
                evhttp_send_reply(req, 206, "breakpoint continuous transmission", NULL); // 区间请求响应的是206
                //断点重传，记录日志

            }
            if(download_path != info.storage_path_)//删除解压产生中间文件
            {
                remove(download_path.c_str());
            }    
        }
        // 上传
        static void Upload(struct evhttp_request *req, void *arg)
        {
            // 约定：请求中包含"low_storage"，说明请求中存在文件数据,并希望普通存储 包含"deep_storage"字段则压缩后存储
            /*
                例：
                有一个 file_1.txt 存成压缩文件 ,file_2.txt 存成普通文件
                /upload/low_storage/file_1.txt 
                /upload/deep_storage/file_2.txt
            */
            //////////获取请求体的内容//////////////
            //获取文件缓冲区对象
            evbuffer* input = evhttp_request_get_input_buffer(req);
            if(!input)
            {
                //获取失败，记录日志

                return;
            }
            //获取请求缓冲区内容长度
            size_t len = evbuffer_get_length(input);
            //记录日志 -缓冲区大小

            //获取上传文件内容
            std::string content(len,'\0');
            if(-1 == evbuffer_copyout(input,content.data(),len))
            {
                //获取失败，记录日志

                return;
            }
            //获取文件名字 客户端自定义请求头 FileName
            std::string filename = evhttp_find_header(req->input_headers,"FileName");
            if(filename.empty())
            {
                //获取失败，记录日志

                evhttp_send_reply(req, HTTP_INTERNAL, NULL, NULL);

                return;
            }
            //解码文件名
            filename = base64_decode(filename);

            // 获取存储类型，客户端自定义请求头 StorageType
            std::string storage_type = evhttp_find_header(req->input_headers,"StorageType");
            //组织存储路径
            std::string storage_path;
            if(storage_type == "low")
            {
                storage_path = storage::Config::GetInstance()->GetLowStorageDir();
            }
            else if(storage_type == "deep")
            {
                storage_path = storage::Config::GetInstance()->GetDeepStorageDir();
            }
            else
            {
                //请求头不存在或者请求头内容无效，记录日志

                evhttp_send_reply(req, HTTP_BADREQUEST, "Illegal storage type", NULL);

                return;
            }

            //创建存储目录
            storage::FileUtil dirCreat(storage_path);
            dirCreat.CreateDirectory();
            
            //组装存储路径
            storage_path+=filename;
            storage::FileUtil sp(storage_path);
            //不需要压缩，直接写文件
            if(storage_path.find(storage::Config::GetInstance()->GetLowStorageDir()) != std::string::npos)
            {
                if(!sp.WriteToFile(content.c_str(),len))
                {
                    //写入失败，记录日志

                    evhttp_send_reply(req, HTTP_INTERNAL, "server error", NULL);

                    return;
                }
                else
                {
                    logsystem::GetLogger("asynclogger")->Info("low_storage success");

                }
            }
            else if(storage_path.find(storage::Config::GetInstance()->GetDeepStorageDir()) != std::string::npos)
            {
                if(!sp.Compress(content,storage::Config::GetInstance()->GetBundleFormat()))
                {
                    //压缩失败，记录日志

                    evhttp_send_reply(req, HTTP_INTERNAL, "server error", NULL);
                    return;
                }
                else
                {
                    logsystem::GetLogger("asynclogger")->Info("deep_storage success");

                }
            }
            
            //上传完成，同步文件信息表
            storage::StorageInfo info;
            info.NewStorageInfo(storage_path);
            data_->Insert(info);
            
            
            evhttp_send_reply(req, HTTP_OK, "Success", NULL);
            logsystem::GetLogger("asynclogger")->Info("upload success");
            
        }

        // 前端代码处理函数
        // 在渲染函数中直接处理StorageInfo
        static std::string generateModernFileList(const std::vector<StorageInfo> &files);

        // 文件大小格式化函数
        static std::string formatSize(uint64_t bytes);

        // 文件列表展示
        static void ListShow(struct evhttp_request *req, void *arg);
        static std::string GetETag(const StorageInfo &info);
        // 下载
    private:
        uint16_t server_port_;
        std::string server_ip_;
        std::string download_prefix_;
        using HandleFunc = std::function<void(evhttp_request *req, void *arg)>;

        // 精确匹配表
        static std::unordered_map<std::string, HandleFunc> exact_routes;
        // 模糊匹配表
        static std::vector<std::pair<std::string, HandleFunc>> prefix_routes;

        // 注册路由表
        void RegisterRoutes()
        {
            // 注册精确匹配表
            exact_routes["/"] = ListShow;
            exact_routes["/upload"] = Upload;

            // 注册模糊匹配表
            prefix_routes.emplace_back(std::make_pair<std::string,HandleFunc>("/download/",Download));
        }
    };

} // namespace storage