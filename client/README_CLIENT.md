# WeakNet DBus 客户端

这是一个基于DBus的网络监控客户端，提供了多种方式来与WeakNet服务通信，获取网络接口信息和监控网络状态变化。

## 特性

- 🔗 与WeakNet DBus服务进行通信
- 📡 实时监控网络状态变化
- 📊 提供网络健康检查功能
- 💾 支持从序列化文件读取离线数据
- 🔧 提供C和C++两种接口

## 编译

```bash
cd /WEAK_NET
make all
```

这会编译服务器和客户端。客户端可执行文件位于 `client/bin/test-client`。

## 使用方法

### 命令行模式

```bash
# 获取当前网络接口信息
make test-client COMMAND=get

# 网络健康检查
make test-client COMMAND=health

# 单次检查网络状态变化
make test-client COMMAND=check

# 持续监听网络变化
make test-client COMMAND=subscribe

# 从文件读取最新状态
make test-client COMMAND=file
```

### C/C++ 库接口

#### 头文件

```cpp
#include "weaknet_client.h"
```

#### C接口函数

```c
// 初始化客户端
bool weaknet_init();

// 清理资源
void weaknet_cleanup();

// 获取网络接口信息
bool weaknet_get_interfaces(char* buffer, size_t buffer_size, 
                           char* error_buffer, size_t error_size);

// 检查网络状态变化（非阻塞）
bool weaknet_check_changes(char* message_buffer, size_t message_size,
                          int32_t* counter, char* error_buffer, size_t error_size);

// 网络健康检查
bool weaknet_health_check(char* result_buffer, size_t result_size,
                         char* error_buffer, size_t error_size);

// 从文件读取状态（离线模式）
bool weaknet_get_from_file(char* buffer, size_t buffer_size,
                          char* error_buffer, size_t error_size);
```

#### C++示例

```cpp
#include "weaknet_client.h"

// 初始化
if (!weaknet_init()) {
    std::cerr << "初始化失败" << std::endl;
    return -1;
}

// 获取网络接口信息
char buffer[1024], error_buffer[256];
if (weaknet_get_interfaces(buffer, sizeof(buffer), error_buffer, sizeof(error_buffer))) {
    std::cout << "网络接口: " << buffer << std::endl;
} else {
    std::cerr << "错误: " << error_buffer << std::endl;
}

// 检查变化
char message[512];
int32_t counter;
if (weaknet_check_changes(message, sizeof(message), &counter, error_buffer, sizeof(error_buffer))) {
    std::cout << "检测到变化: " << message << std::endl;
}

// 清理
weaknet_cleanup();
```

## 客户端类设计

### WeakNetClient 类

客户端提供了完整的C++类封装：

- **连接管理**: 自动管理DBus连接
- **接口调用**: 提供友好的方法调用接口
- **错误处理**: 统一的错误处理和消息返回
- **线程安全**: 支持多线程环境下的安全使用

### 主要方法

- `connect()`: 连接到WeakNet服务
- `getInterfaces()`: 获取网络接口信息
- `subscribeToChanges()`: 订阅网络变化通知
- `checkForChanges()`: 非阻塞检查状态变化
- `requestHealthCheck()`: 请求网络健康检查
- `getLatestFromFile()`: 从文件读取状态

## 集成示例

### Python调用示例

```python
import subprocess
import struct

# 通过C API调用
subprocess.run(['./weaknet-client', 'get'])
```

### Shell脚本集成

```bash
#!/bin/bash

# 检查网络接口
INTERFACES=$(./weaknet-client get)
echo "当前接口: $INTERFACES"

# 监控网络变化
./weaknet-client subscribe &
CLIENT_PID=$!

# 捕获信号时停止客户端
trap "kill $CLIENT_PID" EXIT
```

## 网络状态变化信号

当网络状态发生变化时，服务会发送DBus信号：

- **接口变化**: 网卡添加/移除时
- **TCP丢包率变化**: TCP连接质量变化时
- **RTT变化**: 网络延迟变化时
- **RSSI变化**: Wi-Fi信号强度变化时
- **当前上网网卡变化**: 主网卡切换时

## 错误处理

客户端提供完善的错误处理：

- 连接失败时返回相应错误信息
- 超时处理（2秒超时）
- DBus通信错误处理
- 序列化/反序列化错误处理

## 线程安全

客户端在以下方面保证线程安全：

- DBus连接的安全访问
- 静态变量的线程安全初始化
- 信号处理的安全访问
- 文件操作的安全访问
