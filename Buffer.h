#pragma once 
#include <cstddef>
#include <cstdio>
#include<vector>
#include<string>
//网络库底层的缓冲器类型定义
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
class Buffer
{
public:
    static const size_t kCheapPrepend=8;
    static const size_t kInitialSize=1024;

    explicit Buffer(size_t initilaSize=kInitialSize):
        buffer_(kCheapPrepend+initilaSize),
        readerIndex_(kCheapPrepend),
        writeIndex_(kCheapPrepend)
        {}

    size_t readableBytes() const 
    {
        return writeIndex_-readerIndex_;
    }

    size_t writeableBytes() const{
        return buffer_.size()-writeIndex_;
    }

    size_t prependalbleBytes()const 
    {
        return readerIndex_;
    }

    //返回缓冲区中可读数据的起始地址
    const char* peek()const
    {
        return begin()+readerIndex_;
    }

    //onMessage string <- buffer
    void retrieve(size_t len)
    {
        //表示readerindex只读了len的长度，还剩下readerindex到writeindex
        if(len<readableBytes())
        {
            readerIndex_+=len;
        }
        else //len=readableBytes
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_=writeIndex_=kCheapPrepend;//全部复位
    }
    //onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);
        retrieve(len); //缓冲区复位，因为用了都缓冲区
        return result;
    }

    void ensureWriteableBytes(size_t len)
    {
        if(writeIndex_<len)
        {
            makespace(len);//扩容函数
        }
    }
    //把data和data+len的数据拷贝到缓冲区当中
    void append(const char* data,size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data,data+len,
                    beginWrite());
        writeIndex_+=len;
    }

    char* beginWrite()
    {
        return begin()+writeIndex_;
    }
    const char* beginWrite() const 
    {
        return begin()+writeIndex_;
    }
    
    //封装好的读fd操作
    ssize_t readFd(int fd,int* saveErrno);
    //封装好的写fd操作
    ssize_t writeFd(int fd,int* saveErrno);
private:
    char* begin()
    {
        return &*buffer_.begin();   //vector底层数组首元素的地址
    }
    const char* begin()const{
        return &*buffer_.begin();
    }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writeIndex_;


    void makespace(size_t len)
    {
        /*
         kCheapPrepend | reader | writer   
        */
        
        if(writeableBytes()+prependalbleBytes()<len+kCheapPrepend)
        {
            //缓冲区不够
            buffer_.resize(writeIndex_+len);
        }
        else 
        {
            //缓冲区的长度够，因为读缓冲区少了一部分，可以用，只需要移动数组就可以了
            size_t readable=readableBytes();
            std::copy(begin()+readerIndex_,
                      begin()+writeIndex_,begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writeIndex_=readerIndex_+readable;
        }
    }
};