#pragma once

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/static_thread_pool.hpp>

#include <scene/controller.hpp>

asio::awaitable<void> draw_scene(asio::io_context::executor_type io_exec,
    asio::thread_pool::executor_type pool_exec,
    const scene::controller& controller, const char* wl_display);
