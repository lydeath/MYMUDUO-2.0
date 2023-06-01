#include "FileUtil.h"
#include "Logging.h"
/*
使用 RALL 手法，初始化即构造对象，析构则释放文件资源。
setBuffer 可以设置系统默认的 IO 缓冲区为自己指定的用户缓冲区。大小为 64 KB。
*/

FileUtil::FileUtil(std::string &fileName)
    // 以附加的方式打开只写文件。若文件不存在，则会建立该文件，
    // 如果文件存在，写入的数据会被加到文件尾，即文件原先的内容会被保留。
    : fp_(::fopen(fileName.c_str(), "ae")), // 增补, 如果文件不存在则创建一个
      writtenBytes_(0)
{
    // 将fd_缓冲区设置为本地的buffer_
    /*
    定义函数 void setbuffer (FILE * stream,char * buf,size_t size);
    函数说明 在打开文件流后，读取内容之前，调用 setbuffer () 可用来设置文件流的缓冲区。
    参数 stream 为指定的文件流，参数 buf 指向自定的缓冲区起始地址，
    参数 size 为缓冲区大小。

    */
    ::setbuffer(fp_, buffer_, sizeof(buffer_));
}
FileUtil::~FileUtil()
{
    ::fclose(fp_);
}

void FileUtil::append(const char *data, size_t len)
{
    // 记录已经写入的数据大小
    size_t written = 0;

    while (written != len)
    {
        // 还需写入的数据大小
        size_t remain = len - written;
        size_t n = write(data + written, remain);
        if (n != remain)
        {
            /*
            ferror，函数名，在调用各种输入输出函数（如 putc.getc.fread.fwrite等）时，
            如果出现错误，除了函数返回值有所反映外，还可以用ferror函数检查。 它的
            一般调用形式为 ferror(fp)；如果ferror返回值为0（假），表示未出错。如果
            返回一个非零值，表示出错。应该注意，对同一个文件 每一次调用输入输出函数，
            均产生一个新的ferror函 数值，因此，应当在调用一个输入输出函数后立即检查
            ferror函数的值，否则信息会丢失。在执行fopen函数时，ferror函数的初始值自
            动置为0。
            */
            int err = ferror(fp_);
            if (err)
            {
                fprintf(stderr, "FileUtil::append() failed %s\n", getErrnoMsg(err));
            }
        }
        // 更新写入的数据大小
        written += n;
    }
    // 记录目前为止写入的数据大小，超过限制会滚动日志
    writtenBytes_ += written;
}

void FileUtil::flush()
{
    ::fflush(fp_);
}

size_t FileUtil::write(const char *data, size_t len)
{
    /**
     * size_t fwrite(const void* buffer, size_t size, size_t count, FILE* stream);
     * -- buffer:指向数据块的指针
     * -- size:每个数据的大小，单位为Byte(例如：sizeof(int)就是4)
     * -- count:数据个数
     * -- stream:文件指针
     * 
     * fwrite的线程不安全版本,更快速
     */
    return ::fwrite_unlocked(data, 1, len, fp_);
}