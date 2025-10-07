# WEAK_NET 网络监控项目

一个基于eBPF和DBus的实时网络监控系统，提供网络接口状态监控、流量分析、网络质量评估等功能。

## 🚀 快速开始

### 自动安装

```bash
# 安装依赖并编译
./install.sh

# 仅安装系统依赖
./install.sh --install-deps
```

### 手动安装

```bash
# 1. 安装依赖 (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential clang llvm pkg-config libdbus-1-dev libglog-dev libelf-dev zlib1g-dev libcap-dev linux-headers-$(uname -r) libbpf-dev

# 2. 编译项目
make all

# 3. 启动服务器
./start-server.sh

# 4. 运行测试 (新终端)
./test-client.sh
```

## 📁 项目结构

```
WEAK_NET/
├── server/                 # 服务器端
│   ├── bin/               # 编译产物
│   ├── build/             # eBPF对象文件
│   ├── include/           # 头文件
│   ├── src/               # 源代码
│   └── Makefile           # 服务器Makefile
├── client/                # 客户端
│   ├── bin/               # 编译产物
│   ├── lib/               # 动态库
│   ├── *.cpp              # 客户端源码
│   └── *.h                # 头文件
├── config.mk              # 项目配置
├── Makefile               # 主Makefile
├── install.sh             # 安装脚本
├── start-server.sh        # 服务器启动脚本
├── test-client.sh         # 客户端测试脚本
└── README*.md             # 文档
```

## 🔧 功能特性

### 服务器端功能
- **网络接口监控**: 实时监控网络接口状态变化
- **RTT监控**: 基于ping的网络延迟监控
- **RSSI监控**: Wi-Fi信号强度监控
- **TCP丢包率监控**: 网络连接质量监控
- **流量分析**: 基于eBPF的实时流量分析
- **网络质量评估**: 综合多指标的网络质量评估
- **事件系统**: 基于DBus的事件通知机制

### 客户端功能
- **C/C++ API**: 提供完整的C和C++接口
- **动态库**: 可链接的动态库
- **命令行工具**: 丰富的命令行测试工具
- **事件订阅**: 支持多种网络事件订阅
- **健康检查**: 网络健康状态检查

## 📖 使用方法

### 启动服务器

```bash
# 方式1: 使用启动脚本
./start-server.sh

# 方式2: 直接启动
./server/bin/weaknet-dbus-server

# 方式3: 使用Makefile
make run-server
```

### 客户端测试

```bash
# 运行所有测试
./test-client.sh all

# 获取网络接口信息
./test-client.sh get

# 网络健康检查
./test-client.sh health

# 事件监听测试
./test-client.sh events

# Ping测试
./test-client.sh ping google.com
```

### C/C++ 编程接口

```cpp
#include "client/weaknet_client.h"

// 初始化
if (!weaknet_init()) {
    std::cerr << "初始化失败" << std::endl;
    return -1;
}

// 获取网络接口信息
char buffer[1024], error_buffer[256];
if (weaknet_get_interfaces(buffer, sizeof(buffer), error_buffer, sizeof(error_buffer))) {
    std::cout << "网络接口: " << buffer << std::endl;
}

// 清理
weaknet_cleanup();
```

## 🛠️ 编译选项

```bash
# 编译所有组件
make all

# 仅编译服务器
make server

# 仅编译客户端
make client

# 清理编译产物
make clean

# 运行测试
make test-all
make test-events
make test-performance
```

## 📊 监控指标

### 网络接口指标
- 接口名称和状态
- IP地址和子网掩码
- 网络标志位
- 当前使用状态

### 网络质量指标
- RTT (往返时间)
- TCP丢包率
- RSSI (信号强度)
- 流量统计
- 综合质量评分

### 事件类型
- `InterfaceChanged`: 网络接口变化
- `ConnectionModeChanged`: 上网方式变化
- `NetworkQualityChanged`: 网络质量变化

## 🔍 故障排除

### 常见问题

1. **编译失败**
   ```bash
   # 检查依赖
   ./install.sh --install-deps
   
   # 清理重新编译
   make clean && make all
   ```

2. **服务器启动失败**
   ```bash
   # 检查DBus服务
   systemctl status dbus
   
   # 检查端口占用
   lsof -i :session
   ```

3. **客户端连接失败**
   ```bash
   # 检查服务器是否运行
   pgrep -f weaknet-dbus-server
   
   # 检查DBus连接
   dbus-send --session --dest=com.example.WeakNet --type=method_call --print-reply /com/example/WeakNet com.example.WeakNet.Get
   ```

### 日志文件

- 服务器日志: `./logs/server/`
- 编译日志: 查看终端输出
- 系统日志: `journalctl -f`

## 🤖 AI辅助分析

项目包含强大的Python分析工具，位于 `AI-assisted analysis/` 目录：

### 快速开始

```bash
# 进入AI分析目录
cd "AI-assisted analysis"

# 安装依赖
./install.sh

# 实时监控
python3 realtime_monitor.py --duration 60

# 或直接使用工具
python3 log_parser.py --monitor 60
```

### 主要工具

- **日志解析器**: 解析weaknet-dbus-server终端输出
- **实时监控器**: 实时解析服务器日志并显示指标
- **简单日志读取器**: 最简单的日志解析工具

### 功能特性

- 📊 解析终端输出的网络指标
- 🔍 提取RTT、TCP丢包率、流量等关键信息
- 📁 数据导出（JSON格式）
- 🎯 多种解析方式（完整/实时/简单）

## 📚 文档

- [客户端API文档](client/README_CLIENT.md)
- [动态库使用指南](client/README_LIBRARY.md)
- [事件系统文档](README_EVENTS.md)
- [Ping功能文档](PING_FEATURE_SUMMARY.md)
- [AI辅助分析指南](AI-assisted%20analysis/README.md)

## 🤝 贡献

欢迎提交Issue和Pull Request来改进项目。

## 📄 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 🔗 相关链接

- [eBPF官方文档](https://ebpf.io/)
- [DBus官方文档](https://dbus.freedesktop.org/)
- [libbpf项目](https://github.com/libbpf/libbpf)
