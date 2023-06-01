#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include "MysqlConn.h"
#include "json.hpp"
using json = nlohmann::json;

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>


/*
我们现在封装了 MySQL 连接，现在正式开始封装 MySQL 连接池。
连接池应该做到以下事情，管理连接，获取连接

单例模式
我们只需要一个连接池来管理即可，这里使用单例模式。单例模式
有许多种实现，这里利用 C++ 11 的 static 特性实现单例模式。
C++ 11 保证 static 变量是线程安全的，并且被 static 关键字修饰
的变量只会被创建时初始化，之后都不会。 我们只能通过 
getConnectionPool 静态函数获取唯一的连接池对象，外部不能
调用连接池的构造函数。因此，我们需要将构造函数私有化。相
应的，拷贝构造函数，拷贝赋值运算符以及移动构造函数都不能
被调用。C++ 11 使用 delete 关键字即可实现。
*/
class ConnectionPool
{
public:
    static ConnectionPool *getConnectionPool();
    std::shared_ptr<MysqlConn> getConnection();
    ~ConnectionPool();

private:
    ConnectionPool();
    ConnectionPool(const ConnectionPool &obj) = delete;
    ConnectionPool(const ConnectionPool &&obj) = delete;
    ConnectionPool &operator=(const ConnectionPool &obj) = delete;

    bool parseJsonFile();
    void produceConnection();
    void recycleConnection();
    void addConnection();

    // TODO:加上文件路径
    // std::string filePath_;
    std::string ip_;
    std::string user_;
    std::string passwd_;
    std::string dbName_;
    unsigned short port_;
    int minSize_;
    int maxSize_;
    int currentSize_;
    int timeout_;
    int maxIdleTime_;
    std::queue<MysqlConn *> connectionQueue_; // 连接池队列
    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif
