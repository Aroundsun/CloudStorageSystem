#define DEBUG_LOG
#include "Service.hpp"
#include <thread>
using namespace std;

storage::DataManager *data_;
ThreadPool* tp=nullptr;
logsystem::Config* g_conf_data;
void service_module()
{
    storage::Service s;
    logsystem::GetLogger("asynclogger")->Info("service step in RunModule");
    s.RunModule();
}

void log_system_module_init()
{
    g_conf_data = logsystem::Config::GetInstance();
    tp = new ThreadPool(g_conf_data->thread_count);
    std::shared_ptr<logsystem::LoggerBuilder> Glb(new logsystem::LoggerBuilder());
    Glb->BuildLoggerName("asynclogger");
    Glb->BuildLoggerFlush<logsystem::RollingFileFlush>("./logfile/RollFile_log", 1024 * 1024);
    
    logsystem::LoggerManager::GetInstance().AddLogger(Glb->Build());
}
int main()
{
    log_system_module_init();
    data_ = new storage::DataManager();

    thread t1(service_module);

    t1.join();
    delete(tp);
    return 0;
}