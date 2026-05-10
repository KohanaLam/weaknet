// dbus_service.cpp
// 实现 DBus 服务类：方法处理与信号发送

#include <dbus/dbus.h>
#include <cstdio>
#include <cstring>
#include "logger.hpp"

#include "common.hpp"
#include "serializer.hpp"
#include "server.hpp"
#include "dbus_service.hpp"
#include "weak_netmgr.hpp"
#include "net_info.hpp"
#include "network_quality_assessor.hpp"
#include "net_ping.h"

namespace weaknet_dbus {

DbusService::DbusService(ServerContext* ctx) : ctx_(ctx) {}

// 静态自由函数，转调到对象实例
static DBusHandlerResult MessageHandlerStatic(DBusConnection* conn, DBusMessage* msg, void* user_data) {
    auto* self = reinterpret_cast<DbusService*>(user_data);
    if (!self) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    if (dbus_message_is_method_call(msg, kInterface, kMethodGet)) {
        self->handleGet(conn, msg);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    if (dbus_message_is_method_call(msg, kInterface, kMethodListInterfaces)) {
        self->handleListInterfaces(conn, msg);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    if (dbus_message_is_method_call(msg, kInterface, kMethodGetInterfaces)) {
        self->handleListInterfaces(conn, msg);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    if (dbus_message_is_method_call(msg, kInterface, kMethodHealthCheck)) {
        self->handleHealthCheck(conn, msg);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    if (dbus_message_is_method_call(msg, kInterface, kMethodPing)) {
        self->handlePing(conn, msg);
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

bool DbusService::register_on_connection(DBusConnection* conn) {
    static DBusObjectPathVTable vtable{};
    vtable.message_function = &MessageHandlerStatic;
    return dbus_connection_register_object_path(conn, kObjectPath, &vtable, this);
}

bool DbusService::emitChanged(const std::string& message, int32_t counter) {
    DBusMessage* sig = dbus_message_new_signal(kObjectPath, kInterface, kSignalChanged);
    if (!sig) return false;
    DBusMessageIter args;
    dbus_message_iter_init_append(sig, &args);
    const char* s = message.c_str();
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &s)) { dbus_message_unref(sig); return false; }
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &counter)) { dbus_message_unref(sig); return false; }
    bool ok = dbus_connection_send(ctx_->connection, sig, nullptr);
    dbus_connection_flush(ctx_->connection);
    dbus_message_unref(sig);
    ChangedPayload payload{message, counter};
    std::string err;
    serializeChangedPayloadToFile(payload, kSignalSerializedFile, &err);
    return ok;
}

// MessageHandler 实现已移动到静态自由函数

bool DbusService::handleGet(DBusConnection* conn, DBusMessage* msg) {
    const char* reply_text = "Hello from WeakNet Server";
    DBusMessage* reply = dbus_message_new_method_return(msg);
    if (!reply) return false;
    DBusMessageIter args;
    dbus_message_iter_init_append(reply, &args);
    const char* s = reply_text;
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &s)) { dbus_message_unref(reply); return false; }
    if (!dbus_connection_send(conn, reply, nullptr)) { dbus_message_unref(reply); return false; }
    dbus_connection_flush(conn);
    dbus_message_unref(reply);
    std::string err;
    serializeGetReplyToFile(reply_text, kGetReplySerializedFile, &err);
    return true;
}

bool DbusService::replyStringArray(DBusConnection* conn, DBusMessage* msg, const std::vector<std::string>& arr) {
    DBusMessage* reply = dbus_message_new_method_return(msg);
    if (!reply) return false;
    DBusMessageIter iter;
    dbus_message_iter_init_append(reply, &iter);
    DBusMessageIter array_iter;
    if (!dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &array_iter)) { dbus_message_unref(reply); return false; }
    for (const auto& s : arr) {
        const char* cs = s.c_str();
        if (!dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &cs)) { dbus_message_iter_close_container(&iter, &array_iter); dbus_message_unref(reply); return false; }
    }
    if (!dbus_message_iter_close_container(&iter, &array_iter)) { dbus_message_unref(reply); return false; }
    bool ok = dbus_connection_send(conn, reply, nullptr);
    dbus_connection_flush(conn);
    dbus_message_unref(reply);
    return ok;
}

bool DbusService::handleListInterfaces(DBusConnection* conn, DBusMessage* msg) {
    std::vector<std::string> snapshot;
    {
        std::lock_guard<std::mutex> lk(ctx_->iface_mutex);
        snapshot = WeakNetMgr::namesOf(ctx_->iface_list);
    }
    return replyStringArray(conn, msg, snapshot);
}

bool DbusService::handleHealthCheck(DBusConnection* conn, DBusMessage* msg) {
    std::vector<NetInfo> snapshot;
    {
        std::lock_guard<std::mutex> lk(ctx_->iface_mutex);
        snapshot = ctx_->iface_list;
    }

    NetworkQualityAssessor assessor;
    NetworkQualityResult result = assessor.assessQuality(snapshot);
    std::string reply_text = result.details;

    DBusMessage* reply = dbus_message_new_method_return(msg);
    if (!reply) return false;
    DBusMessageIter args;
    dbus_message_iter_init_append(reply, &args);
    const char* s = reply_text.c_str();
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &s)) { dbus_message_unref(reply); return false; }
    if (!dbus_connection_send(conn, reply, nullptr)) { dbus_message_unref(reply); return false; }
    dbus_connection_flush(conn);
    dbus_message_unref(reply);
    return true;
}

bool DbusService::emitSpecificSignal(const std::string& signalName, const std::string& message, int32_t counter) {
    if (!ctx_ || !ctx_->connection) return false;

    DBusMessage* signal = dbus_message_new_signal(kObjectPath, kInterface, signalName.c_str());
    if (!signal) return false;

    DBusMessageIter iter;
    dbus_message_iter_init_append(signal, &iter);

    const char* msg = message.c_str();
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &msg)) {
        dbus_message_unref(signal);
        return false;
    }

    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &counter)) {
        dbus_message_unref(signal);
        return false;
    }

    bool ok = dbus_connection_send(ctx_->connection, signal, nullptr);
    dbus_connection_flush(ctx_->connection);
    dbus_message_unref(signal);
    
    LOG_INFO(LogModule::DBUS, "emitted signal: " << signalName << ", message='" << message << "', counter=" << counter);
    return ok;
}

bool DbusService::emitNetworkQualitySignal(const std::string& message, const std::string& details, int32_t counter) {
    if (!ctx_ || !ctx_->connection) return false;

    DBusMessage* signal = dbus_message_new_signal(kObjectPath, kInterface, kSignalNetworkQualityChanged);
    if (!signal) return false;

    DBusMessageIter iter;
    dbus_message_iter_init_append(signal, &iter);

    // 添加质量等级参数
    const char* quality = message.c_str();
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &quality)) {
        dbus_message_unref(signal);
        return false;
    }

    // 添加详细信息参数
    const char* details_str = details.c_str();
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &details_str)) {
        dbus_message_unref(signal);
        return false;
    }

    // 添加计数器参数
    if (!dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &counter)) {
        dbus_message_unref(signal);
        return false;
    }

    bool ok = dbus_connection_send(ctx_->connection, signal, nullptr);
    dbus_connection_flush(ctx_->connection);
    dbus_message_unref(signal);
    
    LOG_INFO(LogModule::DBUS, "emitted network quality signal: quality='" << message << "', details='" << details << "', counter=" << counter);
    return ok;
}

bool DbusService::handlePing(DBusConnection* conn, DBusMessage* msg) {
    LOG_INFO(LogModule::DBUS, "handlePing called");
    
    // 解析参数：目标主机名
    DBusError err;
    dbus_error_init(&err);
    const char* hostname = nullptr;
    
    if (!dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &hostname, DBUS_TYPE_INVALID)) {
        LOG_ERROR(LogModule::DBUS, "Ping method error: " << err.message);
        dbus_error_free(&err);
        
        // 发送错误回复
        DBusMessage* reply = dbus_message_new_error(msg, "com.example.WeakNet.Error", "Invalid arguments");
        dbus_connection_send(conn, reply, nullptr);
        dbus_message_unref(reply);
        return false;
    }
    
    if (!hostname || strlen(hostname) == 0) {
        LOG_ERROR(LogModule::DBUS, "Ping method error: empty hostname");
        
        // 发送错误回复
        DBusMessage* reply = dbus_message_new_error(msg, "com.example.WeakNet.Error", "Empty hostname");
        dbus_connection_send(conn, reply, nullptr);
        dbus_message_unref(reply);
        return false;
    }
    
    LOG_INFO(LogModule::DBUS, "Ping request for host: " << hostname);
    
    // 获取当前上网网卡
    std::string currentIface;
    {
        std::lock_guard<std::mutex> lk(ctx_->iface_mutex);
        for (const auto& net : ctx_->iface_list) {
            if (net.usingNow()) {
                currentIface = net.ifName();
                break;
            }
        }
    }
    
    if (currentIface.empty()) {
        LOG_ERROR(LogModule::DBUS, "Ping method error: no active interface found");
        
        // 发送错误回复
        DBusMessage* reply = dbus_message_new_error(msg, "com.example.WeakNet.Error", "No active network interface");
        dbus_connection_send(conn, reply, nullptr);
        dbus_message_unref(reply);
        return false;
    }
    
    LOG_INFO(LogModule::DBUS, "Using interface: " << currentIface << " for ping to " << hostname);
    
    // 调用NetPing进行ping测试
    auto pingInstance = NetPing::getInstance();
    int pingResult = pingInstance->ping(hostname, currentIface, 3000); // 3秒超时
    
    // 构建回复消息
    DBusMessage* reply = dbus_message_new_method_return(msg);
    if (!reply) {
        LOG_ERROR(LogModule::DBUS, "Failed to create ping reply message");
        return false;
    }
    
    DBusMessageIter args;
    dbus_message_iter_init_append(reply, &args);
    
    // 构建结果字符串
    std::string result;
    if (pingResult >= 0) {
        result = std::string("PING ") + hostname + " via " + currentIface + ": " + std::to_string(pingResult) + "ms";
        LOG_INFO(LogModule::DBUS, "Ping successful: " << result);
    } else {
        result = std::string("PING ") + hostname + " via " + currentIface + ": FAILED (error code: " + std::to_string(pingResult) + ")";
        LOG_INFO(LogModule::DBUS, "Ping failed: " << result);
    }
    
    const char* resultStr = result.c_str();
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &resultStr)) {
        LOG_ERROR(LogModule::DBUS, "Failed to append ping result to reply");
        dbus_message_unref(reply);
        return false;
    }
    
    // 发送回复
    bool ok = dbus_connection_send(conn, reply, nullptr);
    dbus_connection_flush(conn);
    dbus_message_unref(reply);
    
    std::printf("[dbus] Ping reply sent: %s\n", ok ? "success" : "failed");
    return ok;
}

}  // namespace weaknet_dbus

