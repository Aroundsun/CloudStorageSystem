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
            
        }
        // 上传
        static void Upload(struct evhttp_request *req, void *arg);

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
            prefix_routes.emplace_back("/download/") = Download;
        }
    };

} // namespace storage