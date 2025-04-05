#ifndef LC_TCP_CONFIG_HPP
#define LC_TCP_CONFIG_HPP

#include "variant_wrapper.hpp"
#include "buffer.hpp"

class TcpSession;
class TcpHandler;

struct TCPReadCommand {};
struct TCPWriteCommand {
    Buffer buffer;
};
struct TCPCloseCommand {};
using TCPCommand = std::variant<TCPReadCommand, TCPWriteCommand, TCPCloseCommand>;
using TCPCommandVariant = VariantWrapper<TCPCommand>;
using TcpSessionPtr = std::shared_ptr<TcpSession>;
using TcpHandlerPtr = std::shared_ptr<TcpHandler>;

inline TcpSession* get_pointer_from(TcpSessionPtr pointer) {
    return pointer.get();
}

#endif // LC_TCP_CONFIG_HPP
