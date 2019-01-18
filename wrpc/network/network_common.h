/**
 * @file network_common.h
 * @author wangcong(a1e2w3@126.com)
 * @date 2018-02-08 15:04:24
 * @brief 
 *  
 **/
 
#ifndef WRPC_NETWORK_NETWORK_COMMON_H_
#define WRPC_NETWORK_NETWORK_COMMON_H_
 
#include <memory>
 
namespace wrpc {
 
class Channel;
typedef std::shared_ptr<Channel> ChannelPtr;
typedef std::weak_ptr<Channel> ChannelWeakPtr;

class Controller;
typedef std::shared_ptr<Controller> ControllerPtr;
typedef std::weak_ptr<Controller> ControllerWeakPtr;

class Connection;
// Connection should not be shared
typedef std::unique_ptr<Connection> ConnectionPtr;
 
} // end namespace wrpc
 
#endif // WRPC_NETWORK_NETWORK_COMMON_H_
 
/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */

