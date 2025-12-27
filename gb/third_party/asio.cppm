// SPDX-License-Identifier: Apache-2.0

/// \file asio.cppm
/// \brief Module wrapper for standalone Asio library
///
/// Provides C++20 module interface to Asio networking library.
/// Uses standalone Asio (not Boost.Asio).
///
/// Asio provides cross-platform async I/O, networking, and timers.
/// Key components:
///   - io_context: Event loop for async operations
///   - tcp::socket, tcp::acceptor: TCP networking
///   - steady_timer: Timers for delayed/periodic operations
///   - async_read, async_write: Async I/O operations
///
/// Example usage:
///   import asio;
///
///   asio::io_context io;
///   asio::steady_timer timer(io);
///   timer.expires_after(std::chrono::seconds(1));
///   timer.async_wait([](asio::error_code ec) {
///       if (!ec) std::println("Timer fired!");
///   });
///   io.run();

module;

#include <boost/asio.hpp>

export module asio;

export namespace asio {
// Core I/O
using boost::asio::dispatch;
using boost::asio::io_context;
using boost::asio::post;
using boost::asio::steady_timer;

// Networking
namespace ip {
using boost::asio::ip::address;
using boost::asio::ip::address_v6;
using boost::asio::ip::tcp;
}  // namespace ip

// Buffers
using boost::asio::buffer;
using boost::asio::const_buffer;
using boost::asio::dynamic_buffer;
using boost::asio::mutable_buffer;
using boost::asio::streambuf;

// Async operations
using boost::asio::async_read;
using boost::asio::async_read_until;
using boost::asio::async_write;

// Error handling
using boost::system::error_code;
using boost::system::system_error;

// Socket options
namespace socket_base {
using reuse_address = boost::asio::socket_base::reuse_address;
using keep_alive = boost::asio::socket_base::keep_alive;
}  // namespace socket_base

// Signal handling
using boost::asio::signal_set;
}  // namespace asio
