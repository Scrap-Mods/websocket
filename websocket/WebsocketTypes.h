#pragma once

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <variant>

typedef websocketpp::client<websocketpp::config::asio_tls_client> tls_client;
typedef websocketpp::client<websocketpp::config::asio_client> non_tls_client;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
typedef std::variant<tls_client::connection_ptr, non_tls_client::connection_ptr> connection_ptr_variant;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
