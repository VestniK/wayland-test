#pragma once

#include <asio/io_service.hpp>
#include <asio/static_thread_pool.hpp>

namespace co {

using pool_executor = asio::thread_pool::executor_type;
using io_executor = asio::io_context::executor_type;

} // namespace co
