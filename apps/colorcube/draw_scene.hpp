#pragma once

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/static_thread_pool.hpp>

#include <libs/corort/executors.hpp>

#include <apps/colorcube/controller.hpp>

asio::awaitable<void> draw_scene(co::io_executor io_exec,
    co::pool_executor pool_exec, const scene::controller& controller,
    const char* wl_display);
