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
 * @brief У��port�Ƿ�Ϸ�(0 < port <= 65535)
 *
 * @param [in] port : int
 *
 * @return bool
 *     true: �Ϸ�
 *     false: ���Ϸ�
 */
bool is_valid_port(uint32_t port);

/**
 * @brief hostnameתip
 *
 * @param [in] hostname : const std::string &
 *
 * @return std::string
 *        ip address if success
 *        "" if failed
 */
std::string hostname_to_ip(const std::string& hostname);

/*
 * @brief ���Խ���socket����
 *        ����ʧ�ܲ�����ر�fd, ���ֶ��ر�
 *
 * @param [in] fd : int
 *        �������ӵ�socket fd
 * @param [in] addr : const struct sockaddr*
 *        ���ӵ�ַ
 * @param [in] timeout_ms : int32_t
 *        ��ʱʱ��, ��λms, <0�����賬ʱ
 *
 * @return int
 *        NET_SUCC: ���ӳɹ�
 *        NET_TIMEOUT: ���ӳ�ʱ
 *        NET_INVALID_ARGUMENT: �������Ϸ�, ��fd < 0
 *        NET_CONNECT_FAIL: ����ʧ��
 */
int connect_with_timeout(int fd, const struct sockaddr* addr, int32_t timeout_ms);

/*
 * @brief �ر�����, close()�ķ�װ
 *
 * @param [in] fd : int
 *
 * @return int
 *        NET_SUCC: �رճɹ�
 *        NET_INTERNAL_ERROR: �ر�ʧ��
 */
int close_connection(int fd);

/**
 * @brief ��fd��ȡָ����Ŀ�ֽ�
 *        ����ֱ�������㹻�ֽ���, ��eof, ���ȡ����, ��ʱ
 *
 * @param [in] fd : int
 *        ����ȡ��fd
 * @param [out] buf : char*
 *        ��Ŷ�ȡ�ֽڵĻ�����, �����С����>=size
 * @param [in] size : size_t
 *        ���Զ�ȡ���ֽ���
 * @param [in] timeout_ms : int32_t
 *        ��ʱ, ��λms, <0��ʾ���賬ʱ
 *
 * @return ssize_t
 *        =0: ֱ�Ӷ���eof, �����ѽ���, �����ݿɶ�
 *        >0: ʵ�ʶ�ȡ�ɹ������ֽ���, ����;����eof, ����ֵ����С��size
 *        <0: ��ȡ�����ʱ
 *            NET_DISCONNECTED: fd < 0, ��Ϊ�����ѱ��ر�
 *            NET_INVALID_ARGUMENT: �������Ϸ�
 *            NET_TIMEOUT: ��ȡ��ʱ
 *            NET_RECV_FAIL: ��ȡ����
 */
ssize_t readn(int fd, char* buf, size_t size, int32_t timeout_ms);

/*
 * @brief ��fd��ȡһ������, ��û���ҵ����л�eof, ����ȡmax_size�ֽ�
 *        ��'\n'��eof��ʶ�н���, ���ݰ���'\n'
 *
 * @param [in] fd : int
 *        ����ȡ��fd
 * @param [out] buf : char*
 *        ��Ŷ�ȡ�ֽڵĻ�����, �����С����>=max_size
 * @param [in] max_size : size_t
 *        ���Զ�ȡ������ֽ���
 * @param [in] timeout_ms : int32_t
 *        ��ʱ, ��λms, <0��ʾ���賬ʱ
 *
 * @return ssize_t
 *        =0: ֱ�Ӷ���eof, �����ѽ���, �����ݿɶ�
 *        >0: ʵ�ʶ�ȡ�ɹ������ֽ���, ����'\n'
 *        <0: ��ȡ�����ʱ
 *            NET_DISCONNECTED: fd < 0, ��Ϊ�����ѱ��ر�
 *            NET_INVALID_ARGUMENT: �������Ϸ�
 *            NET_TIMEOUT: ��ȡ��ʱ
 *            NET_RECV_FAIL: ��ȡ����
 */
ssize_t readline(int fd, char* buf, size_t max_size, int32_t timeout_ms);

/**
 * @brief ��fdд��ָ����Ŀ�ֽ�
 *        ����ֱ��д�����, ��д�����, ��ʱ
 *
 * @param [in] fd : int
 *        ��д���fd
 * @param [in] buf : const char*
 *        ��Ŵ�д�ֽڵĻ�����, �����С����>=size
 * @param [in] size : size_t
 *        ����д����ֽ���
 * @param [in] timeout_ms : int32_t
 *        ��ʱ, ��λms, <0��ʾ���賬ʱ
 *
 * @return ssize_t
 *        >0: ʵ��д��ɹ������ֽ���, ��size
 *        <0: д������ʱ
 *            NET_DISCONNECTED: fd < 0, ��Ϊ�����ѱ��ر�
 *            NET_INVALID_ARGUMENT: �������Ϸ�
 *            NET_TIMEOUT: д�볬ʱ
 *            NET_SEND_FAIL: д�����
 */
size_t writen(int fd, const char* buf, size_t size, int32_t timeout_ms);
 
} // end namespace wrpc
 
#endif // WRPC_UTILS_NET_UTILS_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

