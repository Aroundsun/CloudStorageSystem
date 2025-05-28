#pragma once
#include<thread>
#include<vector>
#include<queue>
#include<mutex>
#include<atomic>
#include<condition_variable>
#include<functional>
#include<iostream>
#include<future>



/*
    * @file threadpool.hpp
    * @brief 线程池类的定义
    * 
    * 该文件定义了一个简单的线程池类，用于管理多个工作线程和任务队列。
    * 线程池可以接收任务并将其分配给工作线程执行，支持多线程并发处理。

*/
class ThreadPool {

public:
    /*
        * @brief 构造函数，初始化线程池
        * @param threads 线程池中线程的数量
        * 
        * 该构造函数创建指定数量的工作线程，并将它们加入到线程池中。
        * 每个工作线程会从任务队列中获取任务并执行，直到线程池被停止。
    */
    ThreadPool(size_t threads):stop(false)
    {
        
    }
    /*
        * @brief 析构函数，停止线程池并清理资源
        * 
        * 该析构函数会停止所有工作线程，并等待它们完成任务。
        * 在销毁线程池之前，确保所有任务都已完成。
    */
    ~ThreadPool()
    {

    }
    /*
        * @brief 添加任务到线程池
        * @param task 要添加的任务，类型为 std::function<void()>
        * 
        * 该函数将任务添加到任务队列中，并通知一个工作线程去执行该任务。
        * 如果线程池已停止，则不会添加新任务。
    */
    template<class F,class... Args>
    auto enqueue(F&& task,Args &&... args)
        -> std::future<typename std::result_of<Args...>::type>
    {
        
    }

private:
    std::vector<std::thread> workers;  //工作线程
    std::atomic<bool> stop;            //线程池是否停止
    std::queue<std::function<void()>> tasks; //任务队列
    std::mutex queue_mutex;            //任务队列的互斥锁
    std::condition_variable condition;  //条件变量，用于通知线程 

};