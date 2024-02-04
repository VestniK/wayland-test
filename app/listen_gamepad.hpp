#pragma once

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

#include <scene/controller.hpp>

asio::awaitable<void> listen_gamepad(
    asio::io_context::executor_type io_exec, scene::controller& controller);
