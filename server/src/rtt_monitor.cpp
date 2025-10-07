// rtt_monitor.cpp
// 基于 NetPing 的 RTT 周期检测，并更新 WeakNetMgr 中 NetInfo 的 rtt 与质量

#include <thread>
#include <chrono>
#include <cstdio>

#include "server.hpp"
#include "dbus_service.hpp"
#include "weak_netmgr.hpp"
#include "rtt_monitor.hpp"
#include "logger.hpp"

using namespace std::chrono_literals;

namespace weaknet_dbus {

void start_rtt_monitor_thread(ServerContext* ctx, const std::string& host, int intervalMs, int timeoutMs) {
    // 简单的后台线程：定期更新 RTT 与质量，并在变化时打印与发信号
    std::thread([ctx, host, intervalMs, timeoutMs]{
        LOG_INFO(LogModule::RTT, "RTT monitor thread started");
        if (!ctx->weak_mgr) ctx->weak_mgr = new WeakNetMgr();
        int loop_count = 0;
        while (ctx->running.load()) {
            loop_count++;
            LOG_INFO(LogModule::RTT, "RTT monitor thread running, loop=" << loop_count << ", ctx->running=" << ctx->running.load());
            try {
                // 直接调用线程安全的RTT更新方法
                LOG_INFO(LogModule::RTT, "RTT monitor: calling updateRttAndStateSafe");
                bool changed = ctx->weak_mgr->updateRttAndStateSafe(host, timeoutMs);
                LOG_INFO(LogModule::RTT, "RTT monitor: updateRttAndStateSafe completed, changed=" << changed);
                
                // 获取当前接口列表用于日志输出
                auto current_interfaces = ctx->weak_mgr->getCurrentInterfaces();
                LOG_INFO(LogModule::RTT, "RTT monitor: current interfaces count=" << current_interfaces.size());
                
                // 输出RTT监控信息
                for (const auto& net : current_interfaces) {
                    LOG_INFO(LogModule::RTT, "RTT_MONITOR: " << net.ifName() 
                        << " | RTT: " << net.rttMs() << "ms" 
                        << " | Quality: " << static_cast<int>(net.quality())
                        << " | Using: " << (net.usingNow() ? "YES" : "NO")
                        << " | Target: " << host);
                }
                
                if (changed && ctx->service) {
                    LOG_INFO(LogModule::RTT, "RTT/Quality updated - emitting signal");
                    // 异步发送DBus信号，避免阻塞RTT线程
                    std::thread([ctx]() {
                        ctx->service->emitChanged("RTT/Quality updated", /*counter*/0);
                    }).detach();
                } else {
                    LOG_INFO(LogModule::RTT, "RTT_MONITOR: no changes detected (interfaces: " << current_interfaces.size() << ")");
                }
                
                LOG_INFO(LogModule::RTT, "RTT monitor: reached sleep preparation");
                LOG_INFO(LogModule::RTT, "RTT monitor: about to sleep for " << intervalMs << "ms");
                std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
                LOG_INFO(LogModule::RTT, "RTT monitor: woke up from sleep");
                
            } catch (const std::exception& e) {
                LOG_ERROR(LogModule::RTT, "RTT monitor thread exception: " << e.what());
            } catch (...) {
                LOG_ERROR(LogModule::RTT, "RTT monitor thread unknown exception");
            }
        }
        LOG_INFO(LogModule::RTT, "RTT monitor thread exiting, ctx->running=" << ctx->running.load());
    }).detach();
}

}  // namespace weaknet_dbus


