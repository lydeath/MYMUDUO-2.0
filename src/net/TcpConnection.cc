#include "TcpConnection.h"

#include "Logging.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL << "mainLoop is null!";
        // LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop)),
      name_(nameArg),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) // 64M
{
    // 下面给channel设置相应的回调函数，poller给channel通知感兴趣的事件发生了，
    //  channel会回调相应的操作函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    LOG_INFO << "TcpConnection::ctor[" << name_.c_str() << "] at fd =" << sockfd;
    // LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);

    // 设置心跳保活机制
    socket_->setKeepAlive(true);
}
TcpConnection::~TcpConnection()
{
    LOG_INFO << "TcpConnection::dtor[" << name_.c_str() << "] at fd=" << channel_->fd() << " state=" << static_cast<int>(state_);
    // LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n",
    //          name_.c_str(), channel_->fd(), (int)state_);
}

// 发送数据     1.loop在当前线程    2.loop不在当前线程
void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected) // tcpconn状态为以连接
    {
        if (loop_->isInLoopThread()) // 如果loop在对应线程
        {
            sendInLoop(buf.c_str(), buf.size()); // 发送数据
        }
        else
        {
            // 遇到重载函数的绑定，可以使用函数指针来指定确切的函数
            void (TcpConnection::*fp)(const void *data, size_t len) = &TcpConnection::sendInLoop;

            loop_->runInLoop(std::bind( // 如果不在，唤醒对应线程，随后对应线程执行回调函数
                fp,
                this,
                buf.c_str(),
                buf.size()));
        }
    }
}

void TcpConnection::send(Buffer *buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            // sendInLoop有多重重载，需要使用函数指针确定
            void (TcpConnection::*fp)(const std::string &message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
        }
    }
}

void TcpConnection::sendInLoop(const std::string &message)
{
    sendInLoop(message.data(), message.size());
}

/**
 * 发送数据  应用写的快， 而内核发送数据慢， 需要把待发送数据写入缓冲区， 而且设置了水位回调
 */
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len; // 没发送完的数据长度
    bool faultError = false;

    // 之前调用过该connection的shutdown，不能再进行发送了
    // 即conn处于断开连接状态
    if (state_ == kDisconnected)
    {
        LOG_ERROR << "disconnected, give up writing";
        // LOG_ERROR("disconnected, give up writing!");
        return;
    }

    // 表示channel_现在不在写数据，而且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len); // 写入数据，返回写入数据大小
        if (nwrote >= 0)                             // 若写入数据
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 既然在这里数据全部发送完成，就不用再给channel设置epollout事件了
                loop_->runInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR << "TcpConnection::sendInLoop";
                // LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE  RESET
                {
                    faultError = true;
                }
            }
        }
    }

    // 说明当前这一次write，并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，然后给channel
    // 注册epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock-channel，调用writeCallback_回调方法
    // 也就是调用TcpConnection::handleWrite方法，把发送缓冲区中的数据全部发送完成
    if (!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余的待发送数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        // 若Buffer缓冲区剩余的待发送数据的长度与尚未读入到缓冲区的数据大于高水位
        // 且Buffer缓冲区剩余的待发送数据的长度小于高水位

        //  即第二缓冲区仍无法处理，调用highWaterMarkCallback_  ？？？？？？？
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        // append函数调用ensureWriteableBytes()调用makespace,为buffer扩展空间装下整个remaining
        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            // 这里一定要注册channel的写事件，否则poller不会给channel通知epollout
            // 当poller发现tcp的发送缓冲区有空间（未写入数据），会通知相应的sock-channel，
            // 调用writeCallback_回调方法也就是调用TcpConnection::handleWrite方法，
            // 更新缓冲区并把发送缓冲区中的数据全部发送完成
            channel_->enableWriting();
        }
    }
}

// 关闭连接
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // 说明outputBuffer中的数据已经全部发送完成
    {
        socket_->shutdownWrite(); // 关闭写端
    }
}

// 连接建立
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 向poller注册channel的epollin事件(读事件)

    // 新连接建立，执行回调
    connectionCallback_(shared_from_this()); // 进行connect
}
// 连接销毁
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();                  // 把channel的所有感兴趣的事件，从poller中del掉
        connectionCallback_(shared_from_this()); // 进行connect
    }
    channel_->remove(); // 把channel从poller中删除掉
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        // LOG_ERROR("TcpConnection::handleRead");
        LOG_ERROR << "TcpConnection::handleRead() failed";
        handleError();
    }
}
void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);              // 复位
            if (outputBuffer_.readableBytes() == 0) // 如果outputBuffer_可读部分为
            {
                channel_->disableWriting(); // 通道设置为不可写
                if (writeCompleteCallback_)
                {
                    // 唤醒loop_对应的thread线程，执行回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnected) // 如果通道写入数据时conn关闭连接，则关闭写端
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            // LOG_ERROR("TcpConnection::handleWrite");
            LOG_ERROR << "TcpConnection::handleWrite() failed";
        }
    }
    else
    {
        // LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
        LOG_ERROR << "TcpConnection fd=" << channel_->fd() << " is down, no more writing";
    }
}
void TcpConnection::handleClose()
{
    // LOG_INFO("TcpConnection::handleClose fd=%d state=%d \n", channel_->fd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // 连接回调
    closeCallback_(connPtr);      // 关闭连接的回调  执行的是TcpServer::removeConnection回调方法
}
void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    // LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d \n", name_.c_str(), err);
    LOG_ERROR << "TcpConnection::handleError name:" << name_.c_str() << " - SO_ERROR:" << err;
}
