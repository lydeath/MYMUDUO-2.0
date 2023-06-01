#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <stdio.h>
#include <string>

/*
AppendFile 在 FileUtil.cc 文件中被实现，其封装了 FILE 对文件操作的方法。
以组合的形式被 LogFile 使用。
*/
class FileUtil
{
public:
    explicit FileUtil(std::string &fileName);
    ~FileUtil();

    void append(const char *data, size_t len);

    void flush();

    off_t writtenBytes() const { return writtenBytes_; }

private:
    size_t write(const char *data, size_t len);

    FILE *fp_;
    char buffer_[64 * 1024]; // fp_的缓冲区
    off_t writtenBytes_;      // off_t用于指示文件的偏移量
};

#endif