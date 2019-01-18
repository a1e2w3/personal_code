/**
 * @file net_utils.cpp
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-12 11:01:23
 * @brief 
 *  
 **/
 
#include "utils/net_utils.h"
 
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils/common_define.h"
#include "utils/timer.h"
#include "utils/write_log.h"
 
namespace wrpc {

bool is_valid_port(uint32_t port) {
    return port > 0 && port <= 65535;
}

std::string hostname_to_ip(const std::string& hostname) {
    struct addrinfo hints;
    struct addrinfo *res = nullptr;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;   // ipv4
    hints.ai_flags = AI_CANONNAME;
    hints.ai_protocol = 0;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
    if (ret != 0 || res == nullptr) {
        WARNING("getaddrinfo for host[%s] fail, errno:[%d:%s]",
                hostname.c_str(), ret, gai_strerror(ret));
        return "";
    }

    char ip_buf[16];
    struct sockaddr_in * addr = reinterpret_cast<struct sockaddr_in *>(res->ai_addr);
    if (nullptr == inet_ntop(AF_INET, &addr->sin_addr, ip_buf, sizeof(ip_buf))) {
        WARNING("inet_ntop for host[%s] fail, errno:[%d:%s]",
                hostname.c_str(), errno, strerror(errno));
        return "";
    }

    freeaddrinfo(res);
    DEBUG("get ip [%s] for host [%s] success.", ip_buf, hostname.c_str());
    return std::string(ip_buf);
}
 
int connect_with_timeout(int fd, const struct sockaddr* addr, int32_t timeout_ms) {
    if (fd < 0 || addr == nullptr) {
        return NET_INVALID_ARGUMENT;
    }

    MillisecondsCountdownTimer timer(timeout_ms);
    MillisecondsCountdownTimer::rep_type remain;
    // set fd non_block
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    int ret = connect(fd, addr, (socklen_t)sizeof(struct sockaddr));
    if (ret == 0) {
        fcntl(fd, F_SETFL, flags);
        return NET_SUCC;
    } else if (ret < 0 && errno != EINPROGRESS) {
        WARNING("connect fd[%d] failed: %d, errno: [%d:%s]", fd, ret, errno, strerror(errno));
        return NET_CONNECT_FAIL;
    }

    if (timer.timeout(&remain)) {
        DEBUG("connect fd[%d] timeout", fd);
        return NET_TIMEOUT;
    }

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL;
    ret = poll(&pfd, 1, remain);
    if (ret < 0) {
        WARNING("connect fd[%d] poll failed: %d, errno: [%d:%s]", fd, ret, errno, strerror(errno));
        return NET_CONNECT_FAIL;
    } else if (ret == 0) {
        DEBUG("connect fd[%d] poll timeout", fd);
        return NET_TIMEOUT;
    } else if ((pfd.revents & POLLIN) != 0 || (pfd.revents & POLLOUT) != 0) {
        int error = 0;
        socklen_t len = sizeof(error);
        ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
        if (ret < 0) {
            WARNING("getsockopt fd[%d] failed: %d, errno: [%d:%s]", fd, ret, errno, strerror(errno));
            return NET_CONNECT_FAIL;
        } else {
            fcntl(fd, F_SETFL, flags);
            return NET_SUCC;
        }
    } else {
        // other events
        WARNING("connect fd[%d] poll got error event:%d, errno: [%d:%s]",
                fd, pfd.revents, errno, strerror(errno));
        return NET_CONNECT_FAIL;
    }
}
 
int close_connection(int fd) {
    if (fd < 0) {
        return NET_SUCC;
    }
    int ret = close(fd);
    if (ret < 0) {
        WARNING("close socket[%d] failed:%d with errno: [%d:%s]", fd, ret,
            errno, strerror(errno));
        return NET_INTERNAL_ERROR;
    }
    return NET_SUCC;
}

ssize_t readn(int fd, char* buf, size_t size, int32_t timeout_ms) {
    if (fd < 0) {
        return NET_DISCONNECTED;
    }
    if (nullptr == buf) {
        return NET_INVALID_ARGUMENT;
    }

    MillisecondsCountdownTimer timer(timeout_ms);
    MillisecondsCountdownTimer::rep_type remain = timeout_ms;

    char* cursor = buf;
    size_t left = size;

    // poll event
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
    while (left > 0) {
        int ret = poll(&pfd, 1, remain);
        if (ret < 0) {
            // error
            if (errno == EINTR) {
                if (timer.timeout(&remain)) {
                    DEBUG("read fd[%d] timeout", fd);
                    return NET_TIMEOUT;
                } else {
                    continue;
                }
            } else {
                WARNING("read fd[%d] poll fail: %d errno: [%d:%s]", fd, ret, errno, strerror(errno));
                return NET_RECV_FAIL;
            }
        } else if (ret == 0) {
            // timeout
            DEBUG("read fd[%d] poll timeout", fd);
            return NET_TIMEOUT;
        } else if ((pfd.revents & POLLIN) != 0) {
            // success read
            ssize_t nread = read(fd, cursor, left);
            if (nread < 0) {
                // error
                if (nread == -1 && errno == EINTR) {
                    continue;
                } else {
                    WARNING("read fd[%d] fail: %d, errno: [%d:%s]", fd, nread, errno, strerror(errno));
                    return NET_RECV_FAIL;
                }
            } else if (nread == 0) {
                // eof
                break;
            } else {
                // success
                cursor += nread;
                left -= nread;
            }
        } else {
            WARNING("read fd[%d] poll got error events %d, errno: [%d:%s]", fd, pfd.revents, errno, strerror(errno));
            return NET_RECV_FAIL;
        }

        if (timer.timeout(&remain)) {
            // timeout
            DEBUG("read fd[%d] timeout", fd);
            return NET_TIMEOUT;
        }
    }
    return static_cast<ssize_t>(size - left);
}

ssize_t readline(int fd, char* buf, size_t max_size, int32_t timeout_ms) {
    if (fd < 0) {
        return NET_DISCONNECTED;
    }
    if (nullptr == buf) {
        return NET_INVALID_ARGUMENT;
    }

    MillisecondsCountdownTimer timer(timeout_ms);
    MillisecondsCountdownTimer::rep_type remain = timeout_ms;

    char* cursor = buf;
    size_t left = max_size;
    bool found_lf = false;

    // poll event
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;
    while (left > 0 && !found_lf) {
        int ret = poll(&pfd, 1, remain);
        if (ret < 0) {
            // error
            if (errno == EINTR) {
                if (timer.timeout(&remain)) {
                    DEBUG("read fd[%d] timeout", fd);
                    return NET_TIMEOUT;
                } else {
                    continue;
                }
            } else {
                WARNING("read fd[%d] poll fail: %d errno: [%d:%s]", fd, ret, errno, strerror(errno));
                return NET_RECV_FAIL;
            }
        } else if (ret == 0) {
            // timeout
            DEBUG("read fd[%d] poll timeout", fd);
            return NET_TIMEOUT;
        } else if ((pfd.revents & POLLIN) != 0) {
            // peek
            ssize_t nread = recv(fd, cursor, left, MSG_PEEK | MSG_NOSIGNAL);
            if (nread < 0) {
                // error
                if (nread == -1 && errno == EINTR) {
                    continue;
                } else {
                    WARNING("read peek fd[%d] fail: %d, errno: [%d:%s]", fd, nread, errno, strerror(errno));
                    return NET_RECV_FAIL;
                }
            } else if (nread == 0) {
                // eof
                found_lf = true;
                break;
            } else {
                // success
                size_t len = nread;
                for (size_t i = 0; i < nread; ++i) {
                    if (cursor[i] == '\n') {
                        len = i + 1;
                        found_lf = true;
                        break;
                    }
                }
                cursor += len;
                left -= len;
                nread = recv(fd, cursor, len, MSG_WAITALL | MSG_NOSIGNAL);
                if (nread != len) {
                    WARNING("read waitall fd[%d] fail: %d, errno: [%d:%s]", fd, nread, errno, strerror(errno));
                    return NET_RECV_FAIL;
                }
            }
        } else {
            WARNING("read fd[%d] poll got error events %d, errno: [%d:%s]", fd, pfd.revents, errno, strerror(errno));
            return NET_RECV_FAIL;
        }

        if (timer.timeout(&remain)) {
            // timeout
            DEBUG("read fd[%d] timeout", fd);
            return NET_TIMEOUT;
        }
    }
    return static_cast<ssize_t>(max_size - left);
}

size_t writen(int fd, const char* buf, size_t size, int32_t timeout_ms) {
    if (fd < 0) {
        return NET_DISCONNECTED;
    }
    if (nullptr == buf || size == 0) {
        return NET_INVALID_ARGUMENT;
    }

    MillisecondsCountdownTimer timer(timeout_ms);
    MillisecondsCountdownTimer::rep_type remain = timeout_ms;

    const char* cursor = buf;
    size_t left = size;

    // poll event
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLOUT | POLLERR | POLLHUP | POLLNVAL;
    while (left > 0) {
        int ret = poll(&pfd, 1, remain);
        if (ret < 0) {
            // error
            if (errno == EINTR) {
                if (timer.timeout(&remain)) {
                    DEBUG("write fd[%d] timeout", fd);
                    return NET_TIMEOUT;
                } else {
                    continue;
                }
            } else {
                WARNING("write fd[%d] poll fail: %d errno: [%d:%s]", fd, ret, errno, strerror(errno));
                return NET_SEND_FAIL;
            }
        } else if (ret == 0) {
            // timeout
            DEBUG("write fd[%d] poll timeout", fd);
            return NET_TIMEOUT;
        } else if ((pfd.revents & POLLOUT) != 0) {
            // success, write
            ssize_t nwrite = write(fd, cursor, left);
            if (nwrite < 0) {
                // error
                if (nwrite == -1 && errno == EINTR) {
                    continue;
                } else {
                    WARNING("write fd[%d] fail: %d, errno: [%d:%s]", fd, nwrite, errno, strerror(errno));
                    return NET_SEND_FAIL;
                }
            } else {
                // success
                cursor += nwrite;
                left -= nwrite;
            }
        } else {
            WARNING("write fd[%d] poll got error events %d, errno: [%d:%s]", fd, pfd.revents, errno, strerror(errno));
            return NET_SEND_FAIL;
        }

        if (timer.timeout(&remain)) {
            // timeout
            DEBUG("write fd[%d] timeout", fd);
            return NET_TIMEOUT;
        }
    }
    return static_cast<ssize_t>(size - left);
}
 
} // end namespace wrpc
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

