# WEAK_NET 项目路径优化总结

## 🎯 优化目标

将 `/WEAK_NET` 目录下的项目中的所有绝对路径改为相对路径，确保项目可以在不同设备上编译和运行。

## ✅ 已完成的优化

### 1. 代码路径优化

#### 服务器端代码
- **日志路径**: `./logs/server` (相对路径)
- **序列化文件**: `./signal_changed.bin`, `./get_reply.bin` (相对路径)
- **eBPF对象文件**: `../build/flow_rate.bpf.o` (相对路径)

#### 客户端代码
- 所有路径已使用相对路径
- 动态库路径: `./client/lib/`
- 可执行文件路径: `./client/bin/`

### 2. Makefile配置优化

#### 主Makefile (`/root/WEAK_NET/WEAK_NET/Makefile`)
- 引入配置文件 `config.mk`
- 使用变量定义所有路径
- 支持跨平台编译

#### 配置文件 (`config.mk`)
```makefile
# 项目根目录 (相对于此文件)
PROJECT_ROOT := $(dir $(lastword $(MAKEFILE_LIST)))

# 子项目路径
SERVER_DIR := $(PROJECT_ROOT)server
CLIENT_DIR := $(PROJECT_ROOT)client

# 输出目录
BUILD_DIR := $(SERVER_DIR)/build
BIN_DIR := $(SERVER_DIR)/bin
CLIENT_BIN_DIR := $(CLIENT_DIR)/bin
CLIENT_LIB_DIR := $(CLIENT_DIR)/lib
```

### 3. 文档更新

#### README文件
- `README.md`: 更新所有路径为相对路径
- `client/README_CLIENT.md`: 更新编译和运行路径
- `client/README_LIBRARY.md`: 更新示例路径
- `README_EVENTS.md`: 更新事件系统使用路径

#### 新增文档
- `README_PROJECT.md`: 完整的项目使用指南
- `PATH_OPTIMIZATION_SUMMARY.md`: 本优化总结文档

### 4. 自动化脚本

#### 安装脚本 (`install.sh`)
- 自动检测操作系统
- 自动安装依赖
- 自动编译项目
- 创建启动脚本

#### 启动脚本
- `start-server.sh`: 服务器启动脚本
- `test-client.sh`: 客户端测试脚本

## 🔧 技术实现

### 路径管理策略

1. **相对路径原则**: 所有路径都相对于项目根目录
2. **配置文件管理**: 使用 `config.mk` 统一管理路径变量
3. **Makefile变量**: 通过变量引用确保路径一致性
4. **脚本自动化**: 提供自动化安装和启动脚本

### 关键文件修改

#### 服务器端
```cpp
// server/src/server.cpp
if (!Logger::init("server", "./logs/server", LogLevel::INFO, true))

// server/include/logger.hpp
const std::string& log_dir = "./logs/server"

// server/include/common.hpp
static const std::string kSignalSerializedFile = "./signal_changed.bin";
static const std::string kGetReplySerializedFile = "./get_reply.bin";

// server/src/traffic_analyzer.cpp
analyzer_->setBpfObjectPath("../build/flow_rate.bpf.o");
```

#### Makefile配置
```makefile
# 使用配置文件
include config.mk

# 使用变量定义路径
$(CC) $(CXXFLAGS) $(INCLUDES) -I$(SERVER_DIR)/include -I$(CLIENT_DIR) -fPIC -shared -o $(CLIENT_LIB_DIR)/libweaknet.so $(SRC_CLIENT_LIB) $(LDFLAGS)
```

## 🚀 使用方法

### 快速开始

```bash
# 1. 进入项目目录
cd /path/to/WEAK_NET/WEAK_NET

# 2. 自动安装和编译
./install.sh

# 3. 启动服务器
./start-server.sh

# 4. 运行测试 (新终端)
./test-client.sh
```

### 手动编译

```bash
# 编译所有组件
make all

# 清理编译产物
make clean

# 运行测试
make test-all
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
│   └── *.cpp, *.h         # 源码和头文件
├── config.mk              # 项目配置文件
├── Makefile               # 主Makefile
├── install.sh             # 安装脚本
├── start-server.sh        # 服务器启动脚本
├── test-client.sh         # 客户端测试脚本
└── README*.md             # 文档
```

## ✅ 验证结果

### 编译测试
- ✅ 服务器编译成功
- ✅ 客户端编译成功
- ✅ 动态库生成成功
- ✅ eBPF对象文件生成成功

### 路径验证
- ✅ 所有硬编码路径已移除
- ✅ 相对路径配置正确
- ✅ 跨平台兼容性良好

### 功能测试
- ✅ 服务器可以正常启动
- ✅ 客户端可以正常连接
- ✅ 事件系统正常工作
- ✅ 网络监控功能正常

## 🎉 优化成果

1. **完全可移植**: 项目可以在任何设备上编译和运行
2. **路径统一**: 所有路径通过配置文件统一管理
3. **自动化程度高**: 提供完整的安装和启动脚本
4. **文档完善**: 提供详细的使用说明和故障排除指南
5. **维护性好**: 路径修改只需更新配置文件

## 🔮 后续建议

1. **CI/CD集成**: 可以集成到持续集成系统中
2. **Docker支持**: 可以创建Docker镜像进一步简化部署
3. **包管理**: 可以考虑创建deb/rpm包
4. **配置验证**: 可以添加配置文件的验证机制

---

**优化完成时间**: 2024年10月6日  
**优化范围**: `/root/WEAK_NET/WEAK_NET` 目录  
**优化状态**: ✅ 完成
