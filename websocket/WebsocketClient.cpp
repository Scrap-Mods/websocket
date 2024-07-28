#include "WebsocketClient.h"

client::connection_ptr WebsocketClient::connect(const std::string& t_uri, websocketpp::lib::error_code& ec)
{
    client::connection_ptr connection = m_client.get_connection(t_uri, ec);
    if (ec) return client::connection_ptr();
        
    m_client.connect(connection);

    return connection;
}

void WebsocketClient::on_tick() {
    m_client.poll();
    m_client.reset();
}

context_ptr WebsocketClient::on_tls_init(websocketpp::connection_hdl) {
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::single_dh_use);


        ctx->set_verify_mode(boost::asio::ssl::verify_none);
    }
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return ctx;
}
