//
// Created by chen on 2022/10/25.
//

#ifndef MYMUDUO_FILEUTIL_H
#define MYMUDUO_FILEUTIL_H

#include "noncopyable.h"
#include "StringPiece.h"
#include <sys/types.h>  // for off_t

namespace muduo {
    namespace FileUtil {

        // read small file < 64KB
        class ReadSmallFile : noncopyable {
        public:
            /*
             *  打开文件，同时设置fd；若出错则记录errno
             */
            ReadSmallFile(StringArg filename);

            /*
             *  关闭文件
             */
            ~ReadSmallFile();

            // return errno
            /*
             *      读文件内容到String中(先读到buffer，然后复制到String)
             *      同时记录文件大小、修改时间和创建时间
             */
            template<typename String>
            int readToString(int maxSize,
                             String *content,
                             int64_t *fileSize,
                             int64_t *modifyTime,
                             int64_t *createTime);

            /// Read at maxium kBufferSize into buf_
            // return errno
            /*
             *      读文件内容到buffer
             */
            int readToBuffer(int *size);

            const char *buffer() const { return buf_; }

            static const int kBufferSize = 64 * 1024;

        private:
            int fd_;
            int err_;
            char buf_[kBufferSize];
        };

        // read the file content, returns errno if error happens.
        // read file content to String
        template<typename String>
        int readFile(StringArg filename,
                     int maxSize,
                     String *content,
                     int64_t *fileSize = nullptr,
                     int64_t *modifyTime = nullptr,
                     int64_t *createTime = nullptr) {
            ReadSmallFile file(filename);
            return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
        }

        // not thread safe
        /*
         *      将日志行追加到文件尾
         *      logline不一定是一行日志，而是整个buffer。
         *      AsyncLogging调用的是：output.append(buffer->data(), buffer->length());       output是LogFile类型，buffer是FixedBuffer类型
         *
         */
        class AppendFile : noncopyable {
        public:
            explicit AppendFile(StringArg filename);

            ~AppendFile();

            void append(const char *logline, size_t len);

            void flush();

            off_t writtenBytes() const { return writtenBytes_; }

        private:

            size_t write(const char *logline, size_t len);

            FILE *fp_;
            char buffer_[64 * 1024];
            off_t writtenBytes_;
        };

    }  // namespace FileUtil
}  // namespace muduo



#endif //MYMUDUO_FILEUTIL_H
