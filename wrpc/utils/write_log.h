/**
 * @file write_log.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-01-26 13:27:54
 * @brief comlog
 *  
 **/
 
#ifndef WRPC_UTILS_WRITE_LOG_H_
#define WRPC_UTILS_WRITE_LOG_H_
 
#include "com_log.h"
 
//#define __CLOSE_TRACE__
//#define __CLOSE_DEBUG__
//#ifdef __DEBUG__
//#undef __DEBUG__
//#endif
//#define __DEBUG__

#define NOTICE_LIGHT(fmt, arg...) \
    do { \
        com_writelog(COMLOG_NOTICE, fmt, ##arg); \
    } while (0)
#ifndef __DEBUG__
#define FATAL(fmt, arg...) \
    do { \
        com_writelog(COMLOG_FATAL, "[%s:%d] "fmt, __FILE__, __LINE__, ##arg); \
    } while (0)
#define WARNING(fmt, arg...) \
    do { \
        com_writelog(COMLOG_WARNING, "[%s:%d] "fmt, __FILE__, __LINE__, ##arg); \
    } while (0)
#define NOTICE(fmt, arg...) \
    do { \
        com_writelog(COMLOG_NOTICE, "[%s:%d] "fmt, __FILE__, __LINE__, ##arg); \
    } while (0)
#else
#define FATAL(fmt, arg...) \
    do { \
        com_writelog(COMLOG_FATAL, "[%s:%d] [%s] "fmt, __FILE__, __LINE__, \
            __PRETTY_FUNCTION__, ##arg); \
    } while (0)
#define WARNING(fmt, arg...) \
    do { \
        com_writelog(COMLOG_WARNING, "[%s:%d] [%s] "fmt, __FILE__, __LINE__, \
                __PRETTY_FUNCTION__, ##arg); \
    } while (0)
#define NOTICE(fmt, arg...) \
    do { \
        com_writelog(COMLOG_NOTICE, "[%s:%d] [%s] "fmt, __FILE__, __LINE__, \
                __PRETTY_FUNCTION__, ##arg); \
    } while (0)
#endif
#ifndef __CLOSE_DEBUG__
#define DEBUG(fmt, arg...) \
    do { \
        com_writelog(COMLOG_DEBUG, "[%s:%d] [%s] "fmt, __FILE__, __LINE__, \
                __PRETTY_FUNCTION__, ##arg); \
    } while (0)
#else
#define DEBUG(fmt, arg...)
#endif
#define R_DEBUG(fmt, arg...) \
    do { \
        com_writelog(COMLOG_DEBUG, "[%s:%d] [%s] "fmt, __FILE__, __LINE__, \
                __PRETTY_FUNCTION__, ##arg); \
    } while (0)
#ifndef __CLOSE_TRACE__
#define TRACE(fmt, arg...) \
    do { \
        com_writelog(COMLOG_TRACE, "[%s:%d] [%s] "fmt, __FILE__, __LINE__, \
                __PRETTY_FUNCTION__, ##arg); \
    } while (0)
#else
#define TRACE(fmt, arg...)
#endif
 
#endif // WRPC_UTILS_WRITE_LOG_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

