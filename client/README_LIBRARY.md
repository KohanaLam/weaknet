# WeakNet 客户端动态库

WeakNet客户端动态库 (`libweaknet.so`) 提供了一个C接口，供其他应用程序调用WeakNet D-Bus服务的功能。

## 📦 库文件

- **动态库**: `client/lib/libweaknet.so`
- **头文件**: `client/weaknet_client.h`
- **测试程序**: `client/bin/test-client`
- **示例程序**: `client/bin/example-client` (编译后)

## 🚀 快速开始

### 1. 编译动态库

```bash
cd /WEAK_NET
make server-client-lib
```

### 2. 基本用法

#### C语言程序示例

```c
#include "weaknet_client.h"

int main() {
    // 初始化
    if (!weaknet_init()) {
        fprintf(stderr, "初始化失败\n");
        return 1;
    }
    
    // 获取网络接口信息
    char buffer[4096], error[256];
    if (weaknet_get_interfaces(buffer, sizeof(buffer), error, sizeof(error))) {
        printf("网络接口: %s\n", buffer);
    }
    
    // Ping指定主机
    if (weaknet_ping_host("google.com", buffer, sizeof(buffer), error, sizeof(error))) {
        printf("Ping结果: %s\n", buffer);
    }
    
    // 清理
    weaknet_cleanup();
    return 0;
}
```

#### C++程序示例

```cpp
#include "weaknet_client.h"
#include <iostream>

void eventCallback(const char* type, const char* message, int32_t counter, const char* source) {
    std::cout << "事件: " << type << " - " << message << std::endl;
}

int main() {
    weaknet_init();
    
    // 订阅事件
    weaknet_subscribe_event("InterfaceChanged", eventCallback);
    
    // 检查事件
    char event_type[64], message[512], source[64];
    int32_t counter;
    char error[256];
    
    if (weaknet_check_events(event_type, sizeof(event_type), message, sizeof(message),
                             &counter, source, sizeof(source), error, sizeof(error))) {
        std::cout << "检测到事件: " << event_type << std::endl;
    }
    
    weaknet_cleanup();
    return 0;
}
```

### 3. 编译用户程序

```bash
# 使用动态库编译用户程序
g++ -std=c++17 -I/path/to/client \
    -L/path/to/client/lib -lweaknet \
    user_program.cpp -o user_program

# 运行时设置库路径
LD_LIBRARY_PATH=/path/to/client/lib:$LD_LIBRARY_PATH \
DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS \
./user_program
```

## 📋 API 参考

### 初始化和清理

```c
bool weaknet_init();                    // 初始化库
void weaknet_cleanup();               // 清理资源
bool weaknet_is_connected();          // 检查连接状态
```

### 信息获取

```c
bool weaknet_get_interfaces(char* buffer, size_t buffer_size, char* error_buffer, size_t error_size);
bool weaknet_health_check(char* result_buffer, size_t result_size, char* error_buffer, size_t error_size);
bool weaknet_get_from_file(char* buffer, size_t buffer_size, char* error_buffer, size_t error_size);
bool weaknet_ping_host(const char* hostname, char* result_buffer, size_t result_size, char* error_buffer, size_t error_size);
```

### 事件系统

```c
typedef void weaknet_event_callback_t(const char* event_type, const char* message, int32_t counter, const char* source);

bool weaknet_subscribe_event(const char* event_type, weaknet_event_callback_t callback);
bool weaknet_unsubscribe_event(const char* event_type);
bool weaknet_get_event_types(char* buffer, size_t buffer_size, char* error_buffer, size_t error_size);
bool weaknet_check_events(char* event_type_buffer, size_t event_type_size,
                          char* message_buffer, size_t message_size,
                          int32_t* counter, char* source_buffer, size_t source_size,
                          char* error_buffer, size_t error_size);
```

### 状态监控

```c
bool weaknet_check_changes(char* message_buffer, size_t message_size, int32_t* counter, char* error_buffer, size_t error_size);
```

## 🧪 测试

```bash
# 测试基本功能
make test-lib

# 测试事件监听
make test-events

# 测试特定功能
make test-client COMMAND=get
make test-client COMMAND=ping google.com
make test-client COMMAND=lib-test
make test-client COMMAND=lib-events
```

## 🔧 支持的事件类型

- `InterfaceChanged` - 网络接口变化
- `ConnectionModeChanged` - 连接方式变化  
- `NetworkQualityChanged` - 网络质量变化

## 📝 使用注意事项

1. **初始化顺序**: 必须先调用 `weaknet_init()` 才能使用其他功能
2. **资源清理**: 程序退出前应调用 `weaknet_cleanup()`
3. **错误处理**: 所有函数返回 `false` 时检查错误缓冲区
4. **线程安全**: 库内部使用DBus，基本是线程安全的
5. **环境依赖**: 需要DBus会话总线运行WeakNet服务器

## 🌐 集成示例

### Python绑定示例

```python
import ctypes

# 加载动态库
lib = ctypes.CDLL("./libweaknet.so")

# 定义C接口函数
lib.weaknet_init.argtypes = []
lib.weaknet_init.restype = ctypes.c_bool

lib.weaknet_get_interfaces.argtypes = [ctypes.c_char_p, ctypes.c_size_t, 
                                       ctypes.c_char_p, ctypes.c_size_t]
lib.weaknet_get_interfaces.restype = ctypes.c_bool

# 使用库
if lib.weaknet_init():
    buffer = ctypes.create_string_buffer(4096)
    error = ctypes.create_string_buffer(256)
    if lib.weaknet_get_interfaces(buffer, 4096, error, 256):
        print(f"网络信息: {buffer.value.decode()}")
```

### Node.js绑定示例

```javascript
const ffi = require('ffi-napi');

const lib = ffi.Library('./libweaknet.so', {
    'weaknet_init': ['bool', []],
    'weaknet_get_interfaces': ['bool', ['string', 'size_t', 'string', 'size_t']],
    'weaknet_cleanup': ['void', []]
});

// 使用库
const buffer = Buffer.alloc(4096);
const error = Buffer.alloc(256);

if (lib.weaknet_init()) {
    if (lib.weaknet_get_interfaces(buffer, 4096, error, 256)) {
        console.log('网络信息:', buffer.toString());
    }
    lib.weaknet_cleanup();
}
```

这样其他的应用程序就可以通过这个动态库轻松集成WeakNet的弱网监控和事件监听功能了！
