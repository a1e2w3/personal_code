/**
 * @file net_utils.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-12 11:01:19
 * @brief 
 *  
 **/
 
#ifndef WRPC_UTILS_NET_UTILS_H_
#define WRPC_UTILS_NET_UTILS_H_
 

#include <stdio.h>
#include <string>
#include <sys/socket.h>
 
namespace wrpc {

/**
 * @brief 校验port是否合法(0 < port <= 65535)
 *
 * @param [in] port : int
 *
 * @return bool
 *     true: 合法
 *     false: 不合法
 */
bool is_valid_port(uint32_t port);

/**
 * @brief hostname转ip
 *
 * @param [in] hostname : const std::string &
 *
 * @return std::string
 *        ip address if success
 *        "" if failed
 */
std::string hostname_to_ip(const std::string& hostname);

/*
 * @brief 尝试建立socket连接
 *        连接失败并不会关闭fd, 需手动关闭
 *
 * @param [in] fd : int
 *        建立连接的socket fd
 * @param [in] addr : const struct sockaddr*
 *        连接地址
 * @param [in] timeout_ms : int32_t
 *        超时时间, 单位ms, <0代表不设超时
 *
 * @return int
 *        NET_SUCC: 连接成功
 *        NET_TIMEOUT: 连接超时
 *        NET_INVALID_ARGUMENT: 参数不合法, 如fd < 0
 *        NET_CONNECT_FAIL: 连接失败
 */
int connect_with_timeout(int fd, const struct sockaddr* addr, int32_t timeout_ms);

/*
 * @brief 关闭连接, close()的封装
 *
 * @param [in] fd : int
 *
 * @return int
 *        NET_SUCC: 关闭成功
 *        NET_INTERNAL_ERROR: 关闭失败
 */
int close_connection(int fd);

/**
 * @brief 从fd读取指定数目字节
 *        阻塞直到读到足够字节数, 或eof, 或读取出错, 或超时
 *
 * @param [in] fd : int
 *        待读取的fd
 * @param [out] buf : char*
 *        存放读取字节的缓冲区, 缓冲大小必须>=size
 * @param [in] size : size_t
 *        尝试读取的字节数
 * @param [in] timeout_ms : int32_t
 *        超时, 单位ms, <0表示不设超时
 *
 * @return ssize_t
 *        =0: 直接读到eof, 数据已结束, 无数据可读
 *        >0: 实际读取成功到的字节数, 若中途读到eof, 返回值可能小于size
 *        <0: 读取出错或超时
 *            NET_DISCONNECTED: fd < 0, 认为连接已被关闭
 *            NET_INVALID_ARGUMENT: 参数不合法
 *            NET_TIMEOUT: 读取超时
 *            NET_RECV_FAIL: 读取出错
 */
ssize_t readn(int fd, char* buf, size_t size, int32_t timeout_ms);

/*
 * @brief 从fd读取一行数据, 若没有找到换行或eof, 最多读取max_size字节
 *        以'\n'或eof标识行结束, 数据包含'\n'
 *
 * @param [in] fd : int
 *        待读取的fd
 * @param [out] buf : char*
 *        存放读取字节的缓冲区, 缓冲大小必须>=max_size
 * @param [in] max_size : size_t
 *        尝试读取的最大字节数
 * @param [in] timeout_ms : int32_t
 *        超时, 单位ms, <0表示不设超时
 *
 * @return ssize_t
 *        =0: 直接读到eof, 数据已结束, 无数据可读
 *        >0: 实际读取成功到的字节数, 包含'\n'
 *        <0: 读取出错或超时
 *            NET_DISCONNECTED: fd < 0, 认为连接已被关闭
 *            NET_INVALID_ARGUMENT: 参数不合法
 *            NET_TIMEOUT: 读取超时
 *            NET_RECV_FAIL: 读取出错
 */
ssize_t readline(int fd, char* buf, size_t max_size, int32_t timeout_ms);

/**
 * @brief 向fd写入指定数目字节
 *        阻塞直到写入完成, 或写入出错, 或超时
 *
 * @param [in] fd : int
 *        待写入的fd
 * @param [in] buf : const char*
 *        存放待写字节的缓冲区, 缓冲大小必须>=size
 * @param [in] size : size_t
 *        尝试写入的字节数
 * @param [in] timeout_ms : int32_t
 *        超时, 单位ms, <0表示不设超时
 *
 * @return ssize_t
 *        >0: 实际写入成功到的字节数, 即size
 *        <0: 写入出错或超时
 *            NET_DISCONNECTED: fd < 0, 认为连接已被关闭
 *            NET_INVALID_ARGUMENT: 参数不合法
 *            NET_TIMEOUT: 写入超时
 *            NET_SEND_FAIL: 写入出错
 */
size_t writen(int fd, const char* buf, size_t size, int32_t timeout_ms);
 
} // end namespace wrpc
 
#endif // WRPC_UTILS_NET_UTILS_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

