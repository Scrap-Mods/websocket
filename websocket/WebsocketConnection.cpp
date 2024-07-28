#include "WebsocketConnection.h"

void WebsocketConnection::on_open(websocketpp::connection_hdl hdl) {
    STORE_STACK(m_state);

    lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_refCallbacks);
    if (lua_type(m_state, -1) != LUA_TTABLE) {
        lua_pop(m_state, 1);
        CHECK_STACK(m_state, 0);
        return;
    }

    lua_getfield(m_state, -1, "open");

    if (lua_type(m_state, -1) != LUA_TFUNCTION) {
        lua_pop(m_state, 2);
        CHECK_STACK(m_state, 0);
        return;
    }

    lua_call(m_state, 0, 0);
    lua_pop(m_state, 1);

    CHECK_STACK(m_state, 0);
}

void WebsocketConnection::on_fail(websocketpp::connection_hdl hdl) {
    STORE_STACK(m_state);
    lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_refCallbacks);
    if (lua_type(m_state, -1) != LUA_TTABLE) {
        lua_pop(m_state, 1);
        CHECK_STACK(m_state, 0);
        return;
    }
    lua_getfield(m_state, -1, "error");

    if (lua_type(m_state, -1) != LUA_TFUNCTION) {
        lua_pop(m_state, 2);
        CHECK_STACK(m_state, 0);
        return;
    }
    
    const auto& ec = m_connection->get_ec();
    lua_pushinteger(m_state, ec.value());
    lua_pushstring(m_state, ec.message().c_str());

    lua_call(m_state, 2, 0);
    lua_pop(m_state, 1);
    CHECK_STACK(m_state, 0);
}

void WebsocketConnection::on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
    STORE_STACK(m_state);
    lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_refCallbacks);
    if (lua_type(m_state, -1) != LUA_TTABLE) {
        lua_pop(m_state, 1);
        CHECK_STACK(m_state, 0);
        return;
    }
    lua_getfield(m_state, -1, "message");

    if (lua_type(m_state, -1) != LUA_TFUNCTION) {
        lua_pop(m_state, 2);
        CHECK_STACK(m_state, 0);
        return;
    }

    lua_pushstring(m_state, msg->get_payload().c_str());

    lua_call(m_state, 1, 0);
    lua_pop(m_state, 1);
    CHECK_STACK(m_state, 0);
}

void WebsocketConnection::on_close(websocketpp::connection_hdl hdl) {
    STORE_STACK(m_state);
    lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_refCallbacks);
    if (lua_type(m_state, -1) != LUA_TTABLE) {
        lua_pop(m_state, 1);
        CHECK_STACK(m_state, 0);
        return;
    }
    lua_getfield(m_state, -1, "close");

    if (lua_type(m_state, -1) != LUA_TFUNCTION) {
        lua_pop(m_state, 2);
        CHECK_STACK(m_state, 0);
        return;
    }

    const auto& ec = m_connection->get_ec();
    lua_pushinteger(m_state, ec.value());
    lua_pushstring(m_state, ec.message().c_str());

    lua_call(m_state, 2, 0);
    lua_pop(m_state, 1);
    CHECK_STACK(m_state, 0);
}
