#pragma once
#include <cstdio>
#include<string>
//定义日志的级别 INFO ERROR FATAL DEBUG
#include "nocopyable.h"

enum LogLevel
{
    INFO, //普通信息
    ERROR,//错误信息
    FATAL,//core信息
    DEBUG,//调试信息
};



//输出一个日志类
class Logger:nocopyable
{
public:
    //获取日志唯一的实例对象
    static Logger& instance();
    //设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);
private:
    int LogLevel_;
    Logger(){}
};

#define LOG_INFO(LogmsgFormat,...) \
    do \
    { \
        Logger& logger=Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024]={0}; \
        snprintf(buf,1024,LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0) \

#define LOG_ERROR(LogmsgFormat,...) \
    do \
    { \
        Logger& logger=Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024]={0}; \
        snprintf(buf,1024,LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    }while(0) \

#define LOG_FATAL(LogmsgFormat,...) \
    do \
    { \
        Logger& logger=Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024]={0}; \
        snprintf(buf,1024,LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    }while(0) \


#ifdef MDEBUG
#define LOG_DEBUG(LogmsgFormat,...) \
    do \
    { \
        Logger& logger=Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024]={0}; \
        sprintf(buf,1024,LogmsgFormat, ##__VA_ARGS__); \
        logger.log(buf) \
    }while(0) \
#else
#define LOG_DEBUG(LogmsgFormat,...)
#endif
