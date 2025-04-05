#ifndef LE_SERVER_HPP
#define LE_SERVER_HPP

#include <cxxAsio.hpp>
#include "tcp_handler.hpp"
#include "udp_handler.hpp"

using ProtocolHandler = std::variant<TcpHandlerPtr, UdpHandlerPtr>;
using ProtocolHandlerVariant = VariantWrapper<ProtocolHandler>;
using ProtocolHandlerConfig = std::variant<TcpConfig, UdpConfig>;
using ProtocolHandlerConfigVariant = VariantWrapper<ProtocolHandlerConfig>;
class ServerConfig final {
    int m_port;
    bool m_v6;
    ProtocolHandlerConfigVariant m_protocol_handler;
public:
    ServerConfig(int port, bool v6, ProtocolHandlerConfigVariant protocol_handler):
    m_port { port },
    m_v6 { v6 },
    m_protocol_handler { std::move( protocol_handler ) } {}

    [[nodiscard]] int port() const { return m_port; }
    [[nodiscard]] bool v6() const { return m_v6; }
    [[nodiscard]] ProtocolHandlerConfigVariant protocol_handler() const { return m_protocol_handler; }
};
class Server;
using ServerConfigPtr = std::shared_ptr<ServerConfig>;

class Server final {
    const ServerConfigPtr m_config;
    asio::io_context& m_io_context;
    asio::strand<asio::any_io_executor>& m_accept_strand;
    ProtocolHandlerVariant m_handler;
    std::function<void()> m_cleanup_action;
    
    void start() {
        m_config->protocol_handler().visit_all_cases(
            [this](const TcpConfig& config) {
                auto handler = std::make_shared<TcpHandler>(m_io_context, config, m_config->port(), m_config->v6());
                m_handler = VariantWrapper<ProtocolHandler> { handler };
                handler->start();
            },
            [this](const UdpConfig& config) {
                auto handler = std::make_shared<UdpHandler>(m_io_context, config, m_config->port(), m_config->v6());
                m_handler = VariantWrapper<ProtocolHandler> { handler };
                handler->start();
            }
        );
    }

public:
    explicit Server(asio::io_context& io_context, 
                   asio::strand<asio::any_io_executor>& accept_strand,
                   ServerConfigPtr config,
                   const std::function<void()>& cleanup_action):
        m_config { std::move(config) },
        m_io_context { io_context },
        m_accept_strand { accept_strand },
        m_cleanup_action { cleanup_action } {
        start();
    }
    
    ~Server() {
        stop();
    }

    void stop() const {
        m_handler.visit_all_cases(
            [](const TcpHandlerPtr &handler) {
                handler->stop();
            },
            [](const UdpHandlerPtr &handler) {
                handler->stop();
            }
        );
        if (m_cleanup_action) {
            m_cleanup_action();
        }
    }

    [[nodiscard]] int port() const {
        return m_config->port();
    }
};

#endif //LE_SERVER_HPP
