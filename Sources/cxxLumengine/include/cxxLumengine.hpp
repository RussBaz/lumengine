#ifndef LUMENGINE_HPP
#define LUMENGINE_HPP

// Following asio suggestion
#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <cxxAsio.hpp>
#include "thread_pool.hpp"
#include "workload.hpp"
#include "custom_error_code.hpp"
#include "buffer.hpp"
#include "custom_terminate_handler.hpp"
#include "swift_function_wrapper.hpp"

#endif // LUMENGINE_HPP
