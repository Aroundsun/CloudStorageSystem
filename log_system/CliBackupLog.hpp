#pragma once
// 远程备份debug等级以上的日志信息-发送端
#include <iostream>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "Util.hpp"

extern logsystem::Config *config;

void start_backup(const std::string &massage)
{
    //创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "start_backup: socket error" << std::endl;
        return;
    }
    //设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET; //IPv4
    server_addr.sin_port = htons(config->backup_port); //端口号
    inet_pton(AF_INET, config->backup_addr.c_str(), &server_addr.sin_addr);
    
    //连接服务器
    int cnt = 5; //重试次数
    while(-1 == connect(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr)))
    {
        if(cnt-- <= 0)
        {
            std::cerr << "start_backup: connect error" << std::endl;
            close(sockfd);
            return;
        }

    }
    //备份数据
    if(send(sockfd, massage.c_str(), massage.size(), 0) < 0)
    {
        std::cerr << "start_backup: send error" << std::endl;
        close(sockfd);
        return;
    }

    close(sockfd); //关闭套接字
}

