# WeakNet 事件系统文档

## 概述

WeakNet 事件系统已经成功实现，为网络状态变化提供了完整的事件监听和通知机制。当网卡进行更换或更上网方式发生改变时，server会发送相应的事件给client，client通过监听接口接收这些事件。

## 🚀 实现的功能

### Server端事件发送

1. **网卡变化事件 (InterfaceChanged)**
   - 网卡添加/移除时触发
   - 发送位置：`start_iface_monitor_thread`
   - 触发条件：检测到网卡列表变化

2. **上网方式改变事件 (ConnectionModeChanged)**
   - 当前上网网卡切换时触发
   - 发送位置：`start_using_iface_thread`
   - 触发条件：检测到主网卡切换

3. **网络质量变化事件 (NetworkQualityChanged)**
   - TCP丢包率、RTT、RSSI等变化时触发
   - 预留接口用于未来扩展

### 事件管理器设计

```cpp
class NetworkEventManager {
public:
    // 便捷的事件发送方法
    void emitInterfaceChanged(const std::string& message, const std::string& source);
    void emitConnectionModeChanged(const std::string& message, const std::string& source);
    void emitNetworkQualityChanged(const std::string& message, const std::string& source);
    
    // 事件回调注册
    void registerCallback(EventType type, EventCallback callback);
    void unregisterCallback(EventType type);
    
    // 事件监控控制
    void startEventMonitoring(struct ServerContext* ctx);
    void stopEventMonitoring();
};
```

### Client端事件监听

#### C接口函数
```c
// 订阅特定事件类型
bool weaknet_subscribe_event(const char* event_type, weaknet_event_callback_t callback);

// 取消订阅事件
bool weaknet_unsubscribe_event(const char* event_type);

// 获取支持的事件类型列表
bool weaknet_get_event_types(char* buffer, size_t buffer_size, char* error_buffer, size_t error_size);

// 非阻塞检查事件
bool weaknet_check_events(char* event_type_buffer, size_t event_type_size,
                         char* message_buffer, size_t message_size, 
                         int32_t* counter, char* source_buffer, size_t source_size,
                         char* error_buffer, size_t error_size);
```

#### 命令行接口
```bash
# 获取支持的事件类型
./weaknet-client event-types

# 订阅特定事件
./weaknet-client event-sub InterfaceChanged

# 检查事件（非阻塞）
./weaknet-client events
```

## 📋 支持的事件类型

1. **InterfaceChanged** - 网卡变化
2. **ConnectionModeChanged** - 上网方式改变
3. **NetworkQualityChanged** - 网络质量变化

## 🔧 使用方法

### 1. 启动服务端
```bash
cd /WEAK_NET
./server/bin/weaknet-dbus-server
```

### 2. 客户端订阅事件
```bash
# 订阅接口变化事件
./client/bin/weaknet-client event-sub InterfaceChanged

# 订阅连接方式变化事件
./client/bin/weaknet-client event-sub ConnectionModeChanged
```

### 3. 监听事件变化
```bash
# 非阻塞检查事件
./client/bin/weaknet-client events

# 获取支持的事件类型
./client/bin/weaknet-client event-types
```

### 4. C程序集成示例
```c
#include "weaknet_client.h"

// 初始化客户端
weaknet_init();

// 订阅事件回调
bool weaknet_subscribe_event("InterfaceChanged", my_event_callback);

// 事件回调函数
void my_event_callback(const char* event_type, const char* message, int32_t counter, const char* source) {
    printf("收到事件: %s - %s\n", event_type, message);
}

// 非阻塞检查事件
char event_type[64], message[512], source[64];
int32_t counter;
char error[256];
if (weaknet_check_events(event_type, 64, message, 512, &counter, source, 64, error, 256)) {
    printf("检测到事件: %s\n", message);
}
```

## 🎯 架构特点

### 1. 事件驱动设计
- **解耦合**: server检测变化，client响应事件
- **异步通知**: 非阻塞的事件检查和通知
- **类型安全**: 强类型的枚举事件定义

### 2. DBus信号机制
- **标准化**: 使用DBus标准信号机制
- **可靠传输**: DBus保证消息传递的可靠性
- **跨进程**: 支持server和client分离部署

### 3. 回调管理
- **灵活注册**: 支持多回调注册
- **类型分类**: 不同事件类型独立管理
- **动态控制**: 运行时订阅/取消订阅

### 4. 优先级系统
- **事件分级**: 0-10优先级（10最高）
- **关键事件**: ConnectionModeChanged优先级9
- **通知事件**: InterfaceChanged优先级8

## 🔍 事件生命周期

```
1. [Server] 检测到网络状态变化
   ↓
2. [EventManager] 创建NetworkEvent
   ↓
3. [EventManager] 调用注册的回调函数
   ↓
4. [DbusService] 发送DBus信号
   ↓
5. [Client] 接收信号并处理
```

## 📊 测试结果

✅ **编译成功**: 所有组件无错误编译  
✅ **事件类型**: 支持3种主要事件类型  
✅ **信号发送**: server成功发送事件信号  
✅ **客户端订阅**: client可以订阅特定事件  
✅ **事件检查**: 非阻塞检查机制工作正常  

## 🔮 未来扩展

1. **更多事件类型**
   - TCP丢包率变化事件
   - RTT延迟变化事件
   - RSSI信号强度变化事件

2. **事件过滤和优先级**
   - 基于优先级的过滤
   - 事件聚合和批处理
   - 自定义事件过滤规则

3. **持久化存储**
   - 事件历史记录
   - 事件统计和分析
   - 事件重放功能

4. **高级集成**
   - WebHook支持
   - RESTful API接口
   - 数据库存储集成

这个事件系统为WeakNet提供了强大的网络状态变化监听能力，能够实时通知客户端网络环境的变化，为上层应用提供了灵活的响应机制。
