//
// Created by Ruslan Bazhenov on 16/02/2025.
//

#ifndef UDP_HANDLER_HPP
#define UDP_HANDLER_HPP

#include "asio.hpp"
#include "buffer.hpp"
#include "swift_function_wrapper.hpp"
#include "variant_wrapper.hpp"

class UdpHandler;
using UdpHandlerPtr = std::shared_ptr<UdpHandler>;

struct UDPReadCommand {};
struct UDPWriteCommand {
    Buffer buffer;
    asio::ip::udp::endpoint endpoint;
};
using UDPCommand = std::variant<UDPReadCommand, UDPWriteCommand>;
using UDPCommandVariant = VariantWrapper<UDPCommand>;

struct UdpConfig {
    uint read_buffer_size { 16 * 1024 };
    SwiftFunctionWrapper<UDPCommandVariant, UdpHandlerPtr, std::error_code, size_t, asio::ip::udp::endpoint> on_receive;
    SwiftFunctionWrapper<UDPCommandVariant, UdpHandlerPtr, std::error_code, size_t> on_write;
    SwiftFunctionWrapper<void, UdpHandlerPtr> on_start;
    SwiftFunctionWrapper<void, UdpHandlerPtr> on_stop;
};

class UdpHandler final : public std::enable_shared_from_this<UdpHandler> {
    const UdpConfig& m_config;
    asio::strand<asio::any_io_executor> m_strand;
    asio::ip::udp::socket m_socket;
    Buffer m_read_buffer;
    asio::ip::udp::endpoint m_sender_endpoint;
    int m_port;

    void handle_command(const UDPCommandVariant &command) {
        command.visit_all_cases(
            [this](const UDPReadCommand&) { read(); },
            [this](const UDPWriteCommand& cmd) { write(cmd.buffer, cmd.endpoint); }
        );
    }

    void read() {
        m_socket.async_receive_from(
            asio::buffer(m_read_buffer.pointer(), m_read_buffer.size()),
            m_sender_endpoint,
            bind_executor(m_strand, [this](std::error_code ec, size_t bytes_transferred) {
                const auto command = m_config.on_receive.call(
                    shared_from_this(),
                    ec,
                    bytes_transferred,
                    m_sender_endpoint
                );
                handle_command(command);
            })
        );
    }

    void write(const Buffer& data, const asio::ip::udp::endpoint& endpoint) {
        m_socket.async_send_to(
            asio::buffer(data.pointer(), data.size()),
            endpoint,
            bind_executor(m_strand, [this](std::error_code ec, size_t bytes_transferred) {
                const auto command = m_config.on_write.call(
                    shared_from_this(),
                    ec,
                    bytes_transferred
                );
                handle_command(command);
            })
        );
    }

public:
    UdpHandler(asio::io_context& io_context, const UdpConfig& config, int const port, const bool v6 = false):
        m_config { config },
        m_strand { asio::make_strand(io_context) },
        m_socket {
            io_context,
            asio::ip::udp::endpoint(
                v6 ? asio::ip::udp::v6() : asio::ip::udp::v4(),
                port
            )
        },
        m_read_buffer { config.read_buffer_size },
        m_port { port } {}

    int port() const {
        return m_port;
    }

    void start() {
        m_config.on_start.call(shared_from_this());
        read();
    }

    void stop() {
        m_socket.close();
        m_config.on_stop.call(shared_from_this());
    }
};

#endif //UDP_HANDLER_HPP
