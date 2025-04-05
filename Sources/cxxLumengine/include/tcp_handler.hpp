#ifndef LE_TCP_HANDLER_HPP
#define LE_TCP_HANDLER_HPP

#include <cxxAsio.hpp>
#include <cxxConfig.hpp>
// #include <handlers-Swift.h>

#include "custom_error_code.hpp"
#include "swift_function_wrapper.hpp"
#include "sparse_vector.hpp"

struct TcpConfig {
    uint read_buffer_size { 16 * 1024 };
    uint pre_allocated_session_count { 128 };
    SwiftFunctionWrapper<TCPCommandVariant, TcpSessionPtr, std::error_code> on_connect;
    SwiftFunctionWrapper<TCPCommandVariant, TcpSessionPtr, std::error_code, size_t> on_receive;
    SwiftFunctionWrapper<TCPCommandVariant, TcpSessionPtr, std::error_code, size_t> on_write;
    SwiftFunctionWrapper<void, TcpSessionPtr, std::error_code> on_disconnect;
    SwiftFunctionWrapper<void, TcpHandlerPtr> on_start;
    SwiftFunctionWrapper<void, TcpHandlerPtr> on_stop;
};
class TcpSession final : public std::enable_shared_from_this<TcpSession> {
    const TcpConfig& m_config;
    asio::ip::tcp::socket m_socket;
    asio::strand<asio::any_io_executor>& m_strand;
    Buffer m_read_buffer;
    std::function<void()> m_clean_up;

    void handle_command(const TCPCommandVariant &command) {
        command.visit_all_cases(
            [this](const TCPReadCommand&) { read(); },
            [this](const TCPWriteCommand& cmd) { write(cmd.buffer); },
            [this](const TCPCloseCommand&) { disconnect(); }
        );
    }

    void read() {
        async_read(m_socket, asio::buffer(m_read_buffer.pointer(), m_read_buffer.size()),
            asio::transfer_at_least(1),
            bind_executor(m_strand, [this](std::error_code ec, const size_t bytes_transferred) {
                const auto command = m_config.on_receive.call(
                    shared_from_this(),
                    ec,
                    bytes_transferred
                );
                handle_command(command);
            })
        );
    }

    void write(const Buffer& data) {
        async_write(m_socket, asio::buffer(data.pointer(), data.size()),
            bind_executor(m_strand, [this](const std::error_code ec, size_t bytes_transferred) {
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
    TcpSession(asio::strand<asio::any_io_executor>& strand, const TcpConfig& config):
        m_config { config },
        m_socket { strand.get_inner_executor() },
        m_strand { strand },
        m_read_buffer { config.read_buffer_size } {}

    ~TcpSession() {
        disconnect();
    }

    void connect(std::error_code ec, std::function<void()> clean_up) {
        m_clean_up = std::move(clean_up);
        const auto command = m_config.on_connect.call(shared_from_this(), ec);
        handle_command(command);
    }

    void disconnect() {
        if (m_socket.is_open()) {
            std::error_code shutdown_ec;
            std::error_code close_ec;
            shutdown_ec = m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, shutdown_ec);
            if (!shutdown_ec) {
                close_ec = m_socket.close(close_ec);
            }
            m_config.on_disconnect.call(shared_from_this(), shutdown_ec ? shutdown_ec : close_ec);
            if (!shutdown_ec && !close_ec && m_clean_up) {
                m_clean_up();
            }
        } else {
            m_config.on_disconnect.call(shared_from_this(), make_error_code(CustomErrorCode::Disconnected));
        }
    }

    [[nodiscard]] asio::ip::tcp::socket& socket() {
        return m_socket;
    }

    TcpSessionPtr static shared(asio::strand<asio::any_io_executor>& strand, const TcpConfig& config) {
        return std::make_shared<TcpSession>(strand, config);
    }
};

class TcpHandler final : public std::enable_shared_from_this<TcpHandler> {
    const TcpConfig& m_config;
    asio::strand<asio::any_io_executor> m_strand;
    asio::ip::tcp::acceptor m_acceptor;
    SparseVector<TcpSessionPtr> m_sessions;
    int m_port;

    void accept() {
        auto session = m_sessions.add(TcpSession::shared(m_strand, m_config));

        m_acceptor.async_accept(
            session->socket(),
            [this, session](const std::error_code ec) {
                session->connect(ec, [this, session] {
                    m_sessions.remove(session);
                });
                if (m_acceptor.is_open()) {
                    accept();
                }
            }
        );
    }

public:
    TcpHandler(asio::io_context& io_context, const TcpConfig& config, int const port, const bool v6 = false):
        m_config { config },
        m_strand { make_strand(io_context) },
        m_acceptor {
            io_context, asio::ip::tcp::endpoint(
                v6 ? asio::ip::tcp::v6() : asio::ip::tcp::v4(),
                port
            )
        },
        m_sessions { config.pre_allocated_session_count },
        m_port { port } {}

    ~TcpHandler() {
        stop();
    }

    [[nodiscard]] int port() const {
        return m_port;
    }

    void start() {
        m_config.on_start.call(shared_from_this());
        accept();
    }

    void stop() {
        m_acceptor.close();
        for (const auto& session : m_sessions) {
            session->disconnect();
        }
        m_config.on_stop.call(shared_from_this());
    }
};

#endif //LE_TCP_HANDLER_HPP
