#include "Logger.hpp"
//g++ ./*.cpp -o build/app -std=c++17 -pthread -ljsoncpp -TEST_LOGGER

ThreadPool *tp = nullptr; // 全局线程池指针
logsystem::Config *config; // 全局配置指针 

void test() {
    int cur_size = 0;
    int cnt = 1;
    while (cur_size++ < 2) {
        logsystem::GetLogger("asynclogger")->Info("测试日志-%d", cnt++);
        logsystem::GetLogger("asynclogger")->Warn("测试日志-%d", cnt++);
        logsystem::GetLogger("asynclogger")->Debug("测试日志-%d", cnt++);
        logsystem::GetLogger("asynclogger")->Error("测试日志-%d", cnt++);
        logsystem::GetLogger("asynclogger")->Fatal("测试日志-%d", cnt++);
    }
}

void init_thread_pool() {
    tp = new ThreadPool(config->thread_count);
}
int main() {
    config = logsystem::Config::GetInstance(); // 获取全局配置实例
    init_thread_pool();
    std::shared_ptr<logsystem::LoggerBuilder> Glb(new logsystem::LoggerBuilder());
    Glb->BuildLoggerName("asynclogger");
    Glb->BuildLoggerFlush<logsystem::FileFlush>("./logfile/FileFlush.log");
    Glb->BuildLoggerFlush<logsystem::RollingFileFlush>("./logfile/RollFile_log",1024 * 1024);
   

    //建造完成后，日志器已经建造，由LoggerManger类成员管理诸多日志器
    // 把日志器给管理对象，调用者通过调用单例管理对象对日志进行落地
    logsystem::LoggerManager::GetInstance().AddLogger(Glb->Build());
    test();
    delete(tp);
    return 0;
}
