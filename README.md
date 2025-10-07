# WEAK_NET DBus 示例（libdbus-1, C++）

本示例在 `com.example.WeakNet` 接口下提供：
- `Get` 方法：返回字符串；并将返回值序列化到 `get_reply.bin`。
- `Changed` 信号：周期性发送，载荷为 `(string message, int32 counter)`；同时将最近一次载荷序列化到 `signal_changed.bin`。

## 目录结构
- `common.hpp`：常量与接口名定义。
- `serializer.hpp/.cpp`：简单二进制序列化/反序列化工具。
- `server.cpp`：DBus 服务端，导出 `Get` 方法并定时发送 `Changed` 信号。
- `client.cpp`：DBus 客户端，调用 `Get` 并监听 `Changed` 信号，同时演示从文件反序列化读取。
- `Makefile`：构建与运行脚本。

## 依赖
- libdbus-1 开发包（Ubuntu/Debian `libdbus-1-dev`）
- pkg-config
- g++

## 构建
```bash
cd /path/to/WEAK_NET/WEAK_NET
make all
```

生成：
- `server/bin/weaknet-dbus-server`
- `client/bin/test-client`

## 运行
确保正在使用会话总线（或根据需要调整为系统总线），在两个终端中分别运行：
```bash
cd /path/to/WEAK_NET/WEAK_NET
make run-server
```
另一个终端：
```bash
cd /path/to/WEAK_NET/WEAK_NET
make run-client
```

如果没有会话总线，可通过 `dbus-launch` 启动：
```bash
export $(dbus-launch)
cd /path/to/WEAK_NET/WEAK_NET
make run-server &
make run-client
```

## 序列化文件
- Get 返回：`./get_reply.bin`
- Changed 载荷：`./signal_changed.bin`

## 注意
- 示例使用会话总线（`DBUS_BUS_SESSION`）。如果要在系统总线运行，请将 `server.cpp` 与 `client.cpp` 中 `dbus_bus_get` 的参数改为 `DBUS_BUS_SYSTEM`，并在系统中正确配置服务名策略。
- 所有函数均含中文注释，说明功能。

