# WeakNet 项目上下文

本文档给后续新对话或编码代理使用，目标是在不重新通读全仓库的情况下快速理解当前项目。这里的信息以当前仓库代码为准，部分旧 README 中的命令或文件名已经不完全准确。

## 项目定位

WeakNet 是一个 Linux 弱网检测与网络诊断项目，主体是 C++17 服务端、C/C++ 客户端动态库和测试工具。服务端通过 DBus 暴露网络状态查询与事件信号，内部使用 netlink、raw ICMP、wpa_supplicant 控制接口、TCP socket diagnostics 和 eBPF/libbpf 采集网络指标。仓库还包含 `AI-assisted analysis/` Python RAG 日志分析工具，用于解析服务端日志并用知识库/大模型生成诊断报告。

核心能力：

- 发现具备上网能力的网卡，并识别当前默认上网网卡。
- 周期性检测 RTT、TCP 重传/丢包率、Wi-Fi RSSI、流量速率和异常流量。
- 综合 RTT、TCP 丢包、RSSI、流量等指标给出网络质量评分。
- 通过 DBus 方法和信号向客户端提供查询、Ping、健康检查和事件通知。
- 通过 `client/lib/libweaknet.so` 暴露 C API，便于其他程序集成。
- 通过 Python 日志捕获和 RAG 工具做弱网日志问答分析。

## 目录地图

- `server/`: C++ DBus 服务端。
- `server/include/`: 服务端头文件和公共 DBus 常量。
- `server/src/`: 服务端实现、监控线程、eBPF 程序和网络采样逻辑。
- `server/bin/weaknet-dbus-server`: 当前仓库已有的服务端二进制产物。
- `server/build/flow_rate.bpf.o`: 当前仓库已有的 eBPF 对象产物。
- `client/`: C API 动态库源码、测试客户端和示例。
- `client/weaknet_client.h`: 对外 C API 头文件。
- `client/client.cpp`: `libweaknet.so` 的主要实现。
- `client/test_client.cpp`: 当前主要命令行测试程序源码。
- `client/lib/libweaknet.so`: 当前仓库已有的客户端动态库产物。
- `AI-assisted analysis/`: Python 日志捕获、FAISS 向量库 RAG、交互式分析工具。
- `Makefile`: 顶层构建、运行和测试入口。
- `server/Makefile`: 服务端和 eBPF 对象构建。
- `config.mk`: 顶层 Makefile 路径变量。
- `install.sh`: 安装依赖、编译项目，并生成 `start-server.sh`、`test-client.sh`。
- `README*.md`、`*_SUMMARY.md`: 历史说明和功能总结，存在部分过期内容。

## 构建与运行

系统依赖主要是：

- `g++`, `clang`, `llvm`, `pkg-config`
- `libdbus-1-dev`, `libglog-dev`
- `libbpf-dev`, `libelf-dev`, `zlib1g-dev`
- `linux-headers-$(uname -r)`
- 运行 raw ICMP/eBPF 功能通常需要 root 或相应 capability，例如 `CAP_NET_RAW`、`CAP_BPF`、`CAP_SYS_ADMIN`。

常用命令：

```bash
make all
make server-client-lib
make clean
make run-server
make -C server
make test-client COMMAND=get
make test-client COMMAND=health
make test-client COMMAND="ping 8.8.8.8"
make test-client COMMAND=event-types
make test-client COMMAND=test-basic
make test-client COMMAND=test-network
make test-client COMMAND=test-ping
make test-client COMMAND=test-events
make test-client COMMAND=test-quality
make test-client COMMAND=test-performance
```

`make run-server` 适合在 `server/bin/weaknet-dbus-server` 已存在时启动服务；如果刚执行过 `make clean`，先运行 `make all` 或 `make -C server`。

DBus 使用 session bus，不是 system bus。没有会话总线时需要先启动，例如：

```bash
export $(dbus-launch)
```

直接运行客户端测试时需要动态库路径：

```bash
LD_LIBRARY_PATH=client/lib:$LD_LIBRARY_PATH ./client/bin/test-client get
```

`install.sh` 会调用 `make clean && make all`，然后生成 `start-server.sh` 和 `test-client.sh`。当前仓库初始状态下这两个脚本不存在，它们是安装脚本的产物。

## DBus 接口

公共常量在 `server/include/common.hpp`。

- Bus name: `com.example.WeakNet`
- Object path: `/com/example/WeakNet`
- Interface: `com.example.WeakNet`

服务端方法：

- `Get`: 返回 `"Hello from WeakNet Server"`，并写入 `./get_reply.bin`。
- `ListInterfaces`: 返回字符串数组。
- `GetInterfaces`: 与 `ListInterfaces` 同义。
- `HealthCheck`: 返回网络质量评估 JSON 字符串。
- `Ping`: 参数为目标主机字符串，返回 `PING host via iface: Xms` 或失败原因。

服务端信号：

- `Changed`: 通用状态变化，载荷为 `(string message, int32 counter)`，并写入 `./signal_changed.bin`。
- `InterfaceChanged`: 网卡变化事件，载荷为 `(string message, int32 counter)`。
- `ConnectionModeChanged`: 当前上网方式变化事件，载荷为 `(string message, int32 counter)`。
- `NetworkQualityChanged`: 网络质量变化事件，载荷为 `(string quality_or_message, string details_json, int32 counter)`。

## 服务端启动流程

入口是 `server/src/main.cpp`，只调用 `weaknet_dbus::start_server()`。

`server/src/server.cpp::start_server()` 的顺序：

1. `Logger::init("server", "./logs/server", LogLevel::INFO, true)` 初始化 glog。
2. `init_dbus()` 连接 DBus session bus，申请 `com.example.WeakNet`，注册对象路径。
3. `getEventManager().startEventMonitoring(&ctx)` 绑定事件管理器。
4. 创建 `WeakNetMgr`，调用 `collectCurrentInterfaces()` 初始化接口列表到 `WeakNetMgr` 内部。
5. 启动以下后台线程：
   - `start_iface_monitor_thread`: 每 10 秒收集可上网接口，检测接口增删。
   - `start_using_iface_thread`: 每 10 秒刷新当前默认上网网卡。
   - `start_rtt_monitor_thread`: 每 10 秒 ping `223.5.5.5`，更新 RTT 和链路质量。
   - `start_rssi_monitor_thread`: 每 10 秒查询 Wi-Fi RSSI。
   - `start_tcp_loss_monitor_thread`: 每 10 秒采样 TCP retrans 统计并计算丢包率。
   - `start_traffic_analysis_thread`: 启动 eBPF 流量分析，先以 `eth0` 作为默认值，再尝试切换为当前 `usingNow()` 接口或已发现的第一个接口。
   - `start_network_quality_thread`: 每 15 秒综合评估网络质量。
6. `Looper::run()` 调用 `dbus_connection_read_write_dispatch(conn, -1)` 阻塞处理 DBus。

服务端目前没有完善的优雅退出逻辑，多数线程常驻或 detach。

## 核心模块

`WeakNetMgr` 是服务端内部指标聚合器，定义在 `server/include/weak_netmgr.hpp`，实现于 `server/src/weak_netmgr.cpp`。它维护线程安全的 `current_interfaces_`，提供接口发现、当前上网网卡标记、RTT/RSSI/TCP 丢包/流量指标更新。

`NetInfo` 定义在 `server/include/net_info.hpp`，是每个接口的核心数据模型，字段包括：

- `ifname`
- `is_default`
- `NetType`: `Unknown`, `Ethernet`, `WiFi`, `Cellular`
- `NetState`: `Down`, `Up`
- `rtt_ms`, `prev_rtt_ms`
- `LinkQuality`: `Unknown`, `Good`, `Fair`, `Poor`, `Bad`
- `rssi_dbm`
- `using_now`
- `tcp_loss_rate`, `tcp_loss_level`
- `traffic_total_bps`, `traffic_total_pps`, `traffic_active_flows`

接口发现：

- `server/src/net_iface.cpp`
- 使用 `NETLINK_ROUTE` dump link 和 IPv4/IPv6 route。
- 判定可上网接口为：非 loopback、UP，并且存在 IPv4 或 IPv6 默认路由且有 gateway。

当前上网网卡：

- `server/src/using_iface.cpp`
- 后台监听 netlink link/route 变化。
- 优先选择有 IPv4 默认路由且 UP 的接口，其次 IPv6。
- 标志位在 `UsingMethodFlag`: `IPv4Default = 0x1`, `IPv6Default = 0x2`。

RTT：

- `server/src/net_ping.cpp`
- 使用 raw ICMP socket，`SO_BINDTODEVICE` 绑定指定接口，只支持 IPv4 解析。
- 返回值为 RTT ms，失败返回负数：`-1` socket 失败，`-2` 绑定网卡失败，`-3` DNS 失败，`-4` 发送失败，`-5` 超时，`-6` select 错误，`-7` 接收失败，`-8` 包格式错误，`-9` 回复验证失败。

RSSI：

- `server/src/net_wifiriss.cpp`
- 通过 wpa_supplicant UNIX DGRAM 控制接口发送 `SIGNAL_POLL`，解析 `RSSI=`。
- 控制目录候选：传入参数、`WPA_CTRL_DIR`、`/run/wpa_supplicant`、`/var/run/wpa_supplicant`。
- 连接失败时会尝试自动拉起 wpa_supplicant，通常需要 root。
- 注意：当前 `collectCurrentInterfaces()` 把接口类型设为 `NetType::Unknown`，因此 RSSI 线程默认会跳过接口，除非后续代码正确设置为 `WiFi`。

TCP 丢包率：

- `server/src/net_tcp.cpp`
- 使用 `NETLINK_SOCK_DIAG` 读取 TCP socket 的 `tcp_info.tcpi_total_retrans`，并用 socket/接口统计近似分母。
- `TcpLossMonitor::compute()` 默认 `minSent=10`，阈值为 degraded `>=1%`，poor `>=5%`。

流量分析：

- `server/src/flow_rate.bpf.c`: eBPF kprobe 程序，统计 IPv4 TCP `ip_queue_xmit` 和 UDP `udp_sendmsg`。
- `server/src/net_traffic.cpp`: libbpf 打开、加载、attach BPF，并从 `current_sec` map 采样 top flows。
- `server/src/traffic_analyzer.cpp`: 管理流量分析线程，设置异常检测参数，缓存实时统计。
- `start_traffic_analysis_thread()` 的初始默认值仍是 `eth0`，但启动时会读取 `WeakNetMgr::getCurrentInterfaces()`，优先使用 `usingNow()` 接口，其次使用已发现列表的第一个接口；如果启动时接口列表为空，才会继续用 `eth0`。
- `TrafficAnalyzer::start()` 会解析 BPF 对象路径，依次尝试 `server/build/flow_rate.bpf.o`、`../build/flow_rate.bpf.o`、`build/flow_rate.bpf.o`，兼容从仓库根目录、`server/bin` 或 `server` 目录启动。

网络质量：

- `server/src/network_quality_assessor.cpp`
- 选择 `usingNow()` 的接口评估，找不到就用第一个接口。
- 总分权重：RTT 30%，TCP 丢包 30%，RSSI 20%，流量 20%。
- 等级：`EXCELLENT >= 90`, `GOOD >= 75`, `FAIR >= 50`, 否则 `POOR`。
- `details` 是 JSON 字符串，包含接口、分数、RTT、丢包、RSSI、流量、活跃连接、质量等级和问题列表。
- 注意：`NetInfo` 默认 RSSI 是 `-1000`，但 `NetworkQualityAssessor::calculateRssiScore()` 只把 `0` 当作无法测量，因此未识别 Wi-Fi 类型或未采到 RSSI 的接口可能会被 RSSI 维度重罚。

事件系统：

- `server/src/event_manager.cpp`
- `NetworkEventManager` 维护回调列表，并通过 `DbusService` 发对应 DBus 信号。
- `InterfaceChanged` 优先级 8，`ConnectionModeChanged` 优先级 9，`NetworkQualityChanged` 优先级 7。

序列化：

- `server/src/serializer.cpp`
- `ChangedPayload` 和 Get 返回值会写入相对路径 `./signal_changed.bin`、`./get_reply.bin`。
- 这些路径都取决于服务端进程当前工作目录。

## 客户端库与测试程序

对外头文件是 `client/weaknet_client.h`，动态库实现是 `client/client.cpp`。

C API：

- `weaknet_init()`
- `weaknet_cleanup()`
- `weaknet_is_connected()`
- `weaknet_get_interfaces(buffer, size, err, err_size)`
- `weaknet_health_check(buffer, size, err, err_size)`
- `weaknet_get_from_file(buffer, size, err, err_size)`
- `weaknet_ping_host(host, buffer, size, err, err_size)`
- `weaknet_check_changes(message, size, counter, err, err_size)`
- `weaknet_subscribe_event(event_type, callback)`
- `weaknet_unsubscribe_event(event_type)`
- `weaknet_get_event_types(buffer, size, err, err_size)`
- `weaknet_check_events(event_type, message, counter, source, err)`
- `weaknet_subscribe_network_quality(callback)`
- `weaknet_check_network_quality(quality, details, counter, err)`
- `weaknet_get_version(buffer, size)`
- `weaknet_get_build_info(buffer, size)`

`client/test_client.cpp` 是当前主要测试入口，构建产物为 `client/bin/test-client`。支持命令：

- `all`
- `get`
- `health`
- `file`
- `ping HOSTNAME`
- `check`
- `subscribe`
- `events`
- `quality`
- `quality-sub`
- `event-types`
- `event-sub EVENT_TYPE`
- `test-basic`
- `test-network`
- `test-ping`
- `test-events`
- `test-quality`
- `test-quality-callback`
- `test-errors`
- `test-performance`

注意事项：

- `weaknet_init()` 只连接 DBus session bus，不保证服务端已经可用；方法调用失败会通过错误缓冲区返回。
- `weaknet_subscribe_event()` 只是添加 DBus match rule，callback 参数当前未真正接入。
- `weaknet_unsubscribe_event()` 当前是简化 no-op，直接返回 true。
- `weaknet_check_events()` 解析普通事件信号为 `(string, int32)`。网络质量信号是 `(string, string, int32)`，应使用 `weaknet_check_network_quality()`。
- `weaknet_subscribe_network_quality()` 内部是阻塞监听循环，不是只注册订阅后立即返回。
- `README_EVENTS.md` 中的 `weaknet-client` 名称是旧文档写法，当前实际构建的命令行程序是 `client/bin/test-client`。

## AI-assisted analysis

Python 分析目录是 `AI-assisted analysis/`。

主要文件：

- `log_capture.py`: 启动 `../server/bin/weaknet-dbus-server` 并解析 stdout/stderr 中的网络指标日志，输出带时间戳的结构化文本。
- `local_vector_rag_analyzer.py`: 推荐的本地向量库 RAG 分析器。使用本地 `SimpleEmbeddings` 和 FAISS，调用阿里百炼 OpenAI-compatible API 的 `qwen-plus` 进行分析。
- `interactive_rag.py`: 交互式入口，可启动日志捕获、列出时间点、分析指定时间点。
- `network_knowledge_base.py`: 网络分析知识库，包含 RTT、TCP 丢包、流量、RSSI、接口状态和常见问题知识。
- `vector_rag_analyzer.py`, `simple_rag_analyzer.py`, `true_rag_analyzer.py`, `optimized_network_rag.py`: 其他版本 RAG 实现或历史实现。
- `network_knowledge_vectorstore_local/index.faiss` 和 `index.pkl`: 本地向量库文件。

Python 依赖见 `AI-assisted analysis/requirements.txt`，包括 `openai`, `langchain`, `faiss-cpu`, `numpy` 等。代码导入了 `langchain_community.vectorstores.FAISS`，实际环境中可能还需要安装 `langchain-community`。

使用前需要配置阿里百炼 API key。推荐入口 `interactive_rag.py` 和 `local_vector_rag_analyzer.py` 已读取 `DASHSCOPE_API_KEY` 环境变量；其他历史脚本中仍有 `YOUR_DASHSCOPE_API_KEY_HERE` 占位符。不要把真实 key 提交进仓库。

`AI-assisted analysis/install_rag.sh` 输出中提到的 `network_diagnosis_tool.py` 当前仓库不存在，这是旧脚本文案。

## 当前已知不一致与风险

这些不是建议，是当前代码阅读得到的事实，后续修改时要优先确认：

- `DbusService::handleListInterfaces()`、`handleHealthCheck()`、`handlePing()` 当前通过 `getInterfaceSnapshot()` 优先读取 `WeakNetMgr::getCurrentInterfaces()`，只有为空时才回退到 `ServerContext::iface_list`。如果这些 DBus 方法返回空，优先检查 `WeakNetMgr` 是否完成接口采集，而不是只看 `ctx.iface_list`。
- `server/src/flow_rate.bpf.c` include `../vmlinux.h`；当前仓库已有 `server/vmlinux.h`，`server/Makefile` 也能通过 `bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h` 生成。`make clean` 会删除 `server/vmlinux.h` 和 `server/build/flow_rate.bpf.o`，重建时需要 `bpftool`、BTF 和 clang BPF 工具链可用。
- `TrafficAnalyzer::start()` 已经有 BPF 对象路径候选列表，但如果从不在候选覆盖范围内的工作目录启动服务端，仍可能找不到对象文件。
- `start_traffic_analysis_thread()` 虽会尝试当前上网接口，但只在启动流量分析器时选择一次；后续路由切换不会自动重新绑定 eBPF 过滤接口。
- `collectCurrentInterfaces()` 目前不识别 Ethernet/WiFi 类型，全部设为 `Unknown`，所以 RSSI 更新默认不会处理无线接口。
- `NetworkQualityAssessor` 对默认 RSSI `-1000` 会按极弱信号计分，非 Wi-Fi 或 RSSI 未采集场景下质量分可能偏低。
- 顶层 README 提到 `start-server.sh`、`test-client.sh`，但它们由 `install.sh` 生成，不是初始仓库文件。
- `README_PROJECT.md` 中提到 `realtime_monitor.py`、`log_parser.py`，当前仓库没有这些文件。
- `README_EVENTS.md` 和部分客户端 README 提到 `weaknet-client`、`example-client`、`lib-test`、`lib-events` 等，当前 Makefile/源码并没有全部生成或支持。
- `client/example_usage.cpp` 当前有明显语法错误 `std::chrono::steady_-clock::now()`，并且不在当前 Makefile 默认构建路径里。
- 仓库包含已构建二进制、动态库、BPF 对象和 `.bin` 序列化文件；项目根目录没有 `.gitignore`。

## 开发约定

- C++ 标准是 C++17，编译参数主要为 `-O2 -Wall -Wextra -Wpedantic`。
- 顶层客户端动态库编译命令包含 `client/client.cpp` 和 `server/src/serializer.cpp`，并 include `server/include` 与 `client`。
- 服务端日志统一使用 `logger.hpp` 的 `LOG_INFO/LOG_ERROR` 宏，底层是 glog。
- DBus 代码直接使用 low-level `libdbus-1` C API。
- Linux 相关采样强依赖内核接口，不适合在非 Linux 环境运行完整功能。
- 涉及 raw socket、SO_BINDTODEVICE、eBPF attach、wpa_supplicant 控制 socket 的测试需要权限和真实网络环境。
- 修改网络指标数据流时，优先检查 `WeakNetMgr::current_interfaces_`，再看 `ServerContext::iface_list` 的回退路径。
- 修改 DBus 信号时，同步更新 `common.hpp`、`dbus_service.cpp`、`client/client.cpp`、`client/weaknet_client.h` 和 `client/test_client.cpp`。
- 修改日志输出格式时，同步更新 `AI-assisted analysis/log_capture.py` 和 `local_vector_rag_analyzer.py` 中的正则。

## 快速调试清单

服务端无法启动：

- 确认 session bus 存在：`echo $DBUS_SESSION_BUS_ADDRESS`。
- 确认依赖库存在：`pkg-config --cflags --libs dbus-1`。
- 查看 `./logs/server` 或服务端 stderr。

客户端连接失败：

- 确认服务端进程存在：`pgrep -f weaknet-dbus-server`。
- 确认使用同一个 `DBUS_SESSION_BUS_ADDRESS`。
- 直接尝试：`make test-client COMMAND=get`。

Ping 失败：

- 确认服务端能看到当前上网接口。
- 确认 raw ICMP 权限。
- 注意当前 `Ping` 方法优先使用 `WeakNetMgr::getCurrentInterfaces()` 快照，若仍提示 `No active network interface`，重点检查接口采集和 `usingNow()` 标记。

eBPF/流量为空：

- 确认 `server/build/flow_rate.bpf.o` 存在。
- 确认服务端启动工作目录能命中 `TrafficAnalyzer::resolveBpfObjectPath()` 的候选路径。
- 确认 libbpf 和内核权限。
- 确认启动时选择的流量分析接口是否是当前上网接口；后续路由切换不会自动重绑。

AI 分析不可用：

- 确认 Python 依赖和 `langchain-community`。
- 确认 API key 没有使用占位符。
- 删除并重建 `network_knowledge_vectorstore_local` 可排查向量库加载问题。
