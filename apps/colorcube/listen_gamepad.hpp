#pragma once

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

#include <scene/controller.hpp>

#include <corort/executors.hpp>

asio::awaitable<void> listen_gamepad(
    co::io_executor io_exec, scene::controller& controller);
