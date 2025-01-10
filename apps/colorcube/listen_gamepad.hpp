#pragma once

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>

#include <libs/corort/executors.hpp>

#include <apps/colorcube/controller.hpp>

asio::awaitable<void> listen_gamepad(co::io_executor io_exec, scene::controller& controller);
