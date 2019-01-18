# wrpc：一个灵活的rpc框架，支持灵活定制应用层协议，名字服务，负载均衡等组件

---
目录结构：
+ common: 共用数据结构定义
+ interface: 策略接口抽象
+ message: 应用层协议，定制消息格式
+ network: 核心的网络通信代码
+ strategy: 定制的策略，包括负载均衡策略，重试策略，名字服务等
+ utils: 共用的工具代码
---
对外接口:
+ Channel: 对应一个下游服务，程序运行过程中可一直保有，配合reloadable工具可支持配置reload
    + init: 初始化，指定服务地址和服务参数
    + create_controller: 创建一个rpc控制器，用于发起一次rpc

+ Controller: 一次rpc过程的控制器，rpc完成后即可删除
    + submit: 发起一次同步rpc请求，阻塞直到rpc结束
    + submit_async: 发起一次异步rpc请求，请求在后台发送，函数立即返回
    + join: 同步等待一个rpc请求结束，阻塞直到rpc结束
    + detach: 让rpc自行在后台进行，应用程序可释放持有的ControllerPtr，rpc不会被取消
    + cancel: 取消正在进行中的rpc
    + status: rpc当前状态
    + get_request/get_response: 获取rpc请求/响应
---
重要的内部概念：
+ EventDispatcher: 基于epoll的事件分发器，每个分发器是一个独立线程，可以配置多个；只做事件监听和分发，没有阻塞操作，保证高并发
+ EndPointManager: 下游状态管理器，管理下游服务每个实例当前状态，包括连接，错误计数，可用状态（alive/death）
+ LoadBalancer: 负载均衡策略抽象，为每一次rpc交互选择一个下游实例，并接收每次网络请求的反馈，监听下游服务列表的变更，及时更新内部状态，首次请求和重试请求使用相同接口配置不同策略
+ NamingService: 名字服务抽象，定期解析naming，刷新下游列表，检查下游实例健康状态
