#pragma once

#include <asio/static_thread_pool.hpp>
#include <asio/io_service.hpp>

namespace co {

using pool_executor = asio::thread_pool::executor_type;
using io_executor = asio::io_context::executor_type;

}
