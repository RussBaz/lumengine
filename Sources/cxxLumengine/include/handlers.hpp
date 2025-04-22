#ifndef LE_HANDLERS_HPP
#define LE_HANDLERS_HPP

#include "tcp_handler.hpp"
#include "udp_handler.hpp"

extern "C" {
    // TCP
    typedef TCPCommandVariant (*tcp_on_connect_handler)(TcpSessionPtr, std::error_code);
    typedef TCPCommandVariant (*tcp_on_receive_handler)(TcpSessionPtr, std::error_code, size_t);
    typedef TCPCommandVariant (*tcp_on_write_handler)(TcpSessionPtr, std::error_code, size_t);
    typedef void (*tcp_on_disconnect_handler)(TcpSessionPtr, std::error_code);
    typedef void (*tcp_on_start_handler)(TcpHandlerPtr);
    typedef void (*tcp_on_stop_handler)(TcpHandlerPtr);
    // UDP
    typedef UDPCommandVariant (*udp_on_receive_handler)(UdpHandlerPtr, std::error_code, size_t, asio::ip::udp::endpoint);
    typedef UDPCommandVariant (*udp_on_write_handler)(UdpHandlerPtr, std::error_code, size_t);
    typedef void (*udp_on_start_handler)(UdpHandlerPtr);
    typedef void (*udp_on_stop_handler)(UdpHandlerPtr);
}

inline TcpSession* get_tcp_session_unsafe(TcpSessionPtr pointer) {
    return pointer.get();
}
inline TcpHandler* getTcpHandlerUnsafe(TcpHandlerPtr pointer) {
    return pointer.get();
}
inline UdpHandler* getUdpHandlerPointer(UdpHandlerPtr pointer) {
    return pointer.get();
}

enum class HandlerType {
    TcpOnConnect,
    TcpOnRceive,
    TcpOnWrite,
    TcpOnDisconnect,
    TcpOnStart,
    TcpOnStop,
    UpdOnReceive,
    UpdOnWrite,
    UdpOnStart,
    UdpOnStop,
};

// Memory management
typedef void (*MyCallbackType)(int);
extern "C" void release_swift_context(void *context, HandlerType);
extern "C" MyCallbackType create_swift_callback();

#endif // LE_HANDLERS_HPP
