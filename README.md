# CloudStorageSystem

## 项目简介
本项目是一个基于C++实现的简易云存储系统，支持文件的上传、下载、列表展示，并具备异步日志系统。服务端支持网页交互，命令行交互没有实现

## 主要功能
- 文件上传/下载/列表
- 支持普通存储和压缩存储
- 支持网页端操作（HTML页面）
- 异步日志记录与备份

## 目录结构
```
CloudStorageSystem/
├── log_system/                # 日志系统相关代码
│   ├── AsyncBuffer.hpp        # 日志缓冲区实现
│   ├── AsyncLogger.hpp        # 异步日志器实现
│   ├── AsyncWorker.hpp        # 日志异步写入线程
│   ├── CliBackupLog.hpp       # 日志备份客户端
│   ├── config.conf            # 日志系统配置文件
│   ├── LogFlush.hpp           # 日志刷新策略
│   ├── Logger.hpp             # 日志接口与宏
│   ├── LogLevel.hpp           # 日志等级定义
│   ├── Manager.hpp            # 日志管理器
│   ├── Message.hpp            # 日志消息结构
│   ├── ServerBackupLog.cpp    # 日志备份服务端
│   ├── Threadpool.hpp         # 线程池实现
│   ├── Util.hpp               # 日志系统工具函数
│   └── ...                    # 其他日志相关文件
│
├── src/
│   ├── client/                # 客户端代码（暂未实现）
│   │   
│   │
│   └── server/                # 服务端代码
│       ├── base64.cpp/.h      # base64 编解码实现
│       ├── bundle.h           # 压缩/解压相关接口
│       ├── Config.hpp         # 服务端配置管理
│       ├── DataManager.hpp    # 文件元数据管理
│       ├── html_page.html     # 服务端网页（上传/下载/列表）
│       ├── Service.hpp        # HTTP服务主逻辑
│       ├── storage.conf       # 服务端配置文件
│       ├── Test.hpp           # 服务端测试/入口（含main函数，建议迁移到main.cpp）
│       ├── Util.hpp           # 服务端工具函数
│       └── CMakeLists.txt     # 服务端编译脚本
│
├── README.md                  # 项目说明文档
└── ...                        # 其他文件
```

## 依赖说明
- C++17 标准
- [libevent](http://libevent.org/)
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp)
- pthread
- stdc++fs
- bundle（第三方压缩库）

## 编译与运行

### 服务端
1. 进入 `src/server/` 目录：
   ```sh
   cd src/server
   mkdir build && cd build
   cmake ..
   make
   ```
2. 运行服务端：
   ```sh
   ./server
   ```
3. 浏览器访问 `http://127.0.0.1:8081/`，即可使用网页上传/下载/查看文件。



## 配置文件
- 服务端配置：`src/server/storage.conf`
- 日志系统配置：`log_system/config.conf`

## 常见问题
- 依赖库未安装请先用包管理器安装（如 `brew install libevent jsoncpp`）。
- 端口、存储路径等可在配置文件中修改。

