<p align="center">
  <img src="https://github.com/user-attachments/assets/0c125c4c-ac23-4194-90fa-555817618bf6"/>
</p>

# Websocket Library for Lua

This library provides a simple interface for working with WebSockets in Lua. It allows you to create WebSocket connections, send and receive messages, and handle various events.

![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/Scrap-Mods/websocket/total)
![GitHub tag check runs](https://img.shields.io/github/check-runs/scrap-mods/websocket/v1.0.0)
[![Discord](https://img.shields.io/discord/944260227195351040?link=https%3A%2F%2Fdiscord.gg%2FahzyHPn3y2)](https://discord.gg/ahzyHPn3y2)

## Features

- Easy-to-use WebSocket connection management
- Event-based handling for open, close, error, and message events
- Support for optional headers and cookies
- Asynchronous operation with a tick-based update system

## Installation

To use this WebSocket library, follow these steps:

1. Download the `.dll` files from this repository.

2. Place the `.dll` files in the appropriate directory for your game. For example:
   - For Scrap Mechanic: `C:\Program Files (x86)\Steam\steamapps\common\Scrap Mechanic\Release`
   - For other games, consult the game's modding documentation to find the correct directory

3. Once the DLL is in place, you can import the WebSocket library in your Lua script using:

## Usage

Here's a basic example of how to use the websocket library:

```lua
local ws = require("websocket")

-- Optional headers and cookies
local optional_headers = {A = "Hello", B = "World"}
local optional_cookies = {Cookie_1 = "Value_1"}

-- Create a connection
local connection = ws:connect("wss://127.0.0.1:8080", optional_headers, optional_cookies)

-- Event handlers
connection:on("open", function()
    print("Opened connection")
    connection:send("Waddup bro")
end)

connection:on("close", function(code, reason)
    print("Closed connection", code, reason)
end)

connection:on("error", function(code, reason)
    print("Connection errored", code, reason)
end)

connection:on("message", function(data)
    print("message: ", data)
    connection:send(data .. tostring(math.random(9)))
end)

-- In your main loop or update function, call:
ws:tick()

-- To close the connection:
connection:close()

```

## API Reference

### `ws:connect(url, headers, cookies)`

Establishes a WebSocket connection.

- `url`: The WebSocket server URL (e.g., "wss://127.0.0.1:8080")
- `headers`: (optional) A table of additional headers
- `cookies`: (optional) A table of cookies

Returns a connection object.

### `connection:on(event, callback)`

Registers an event handler.

- `event`: String, one of "open", "close", "error", or "message"
- `callback`: Function to be called when the event occurs

### `connection:send(data)`

Sends data through the WebSocket connection.

- `data`: The data to send

### `connection:close()`

Closes the WebSocket connection.

### `ws:tick()`

Updates the WebSocket system. Should be called regularly (e.g., in a game loop or update function).
