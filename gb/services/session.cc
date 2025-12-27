// SPDX-License-Identifier: Apache-2.0

module;

#include <cstdio>

import asio;
import std;

module session;

Session::Session(asio::ip::tcp::socket socket, EntityManager& em,
                 std::function<void(std::shared_ptr<Session>)> on_disconnect)
    : socket_(std::move(socket)), entity_manager_(em),
      on_disconnect_(std::move(on_disconnect)) {
  // Log connection (get peer address)
  auto endpoint = socket_.remote_endpoint();
  std::println(stderr, "NEW CONNECTION from {}",
               endpoint.address().to_string());
}

void Session::start() {
  do_read();
}

void Session::do_read() {
  auto self = shared_from_this();
  asio::async_read_until(
      socket_, input_buffer_, '\n',
      [this, self](asio::error_code ec, std::size_t /*bytes_transferred*/) {
        if (ec) {
          disconnect();
          return;
        }

        // Check for input flooding
        if (input_queue_.size() >= MAX_INPUT_QUEUE_SIZE) {
          std::println(stderr,
                       "Disconnecting flooding client (input queue overflow)");
          disconnect();
          return;
        }

        // Extract line from buffer
        std::istream is(&input_buffer_);
        std::string line;
        std::getline(is, line);
        // Remove trailing \r if present (Windows clients)
        if (!line.empty() && line.back() == '\r') line.pop_back();
        // Add to input queue for command processing
        if (!line.empty()) {
          input_queue_.push_back(std::move(line));
        }
        // Continue reading
        do_read();
      });
}

bool Session::has_pending_output() const {
  // Check if ostringstream has content by checking if str() is empty
  return !out_buffer_.str().empty();
}

std::size_t Session::write_queue_size() const {
  std::size_t total = 0;
  for (const auto& s : write_queue_)
    total += s.size();
  return total;
}

void Session::flush_to_network() {
  if (!has_pending_output()) return;

  // Check for slow client - if queue is too large, disconnect
  if (write_queue_size() > MAX_WRITE_QUEUE_SIZE) {
    std::println(stderr, "Disconnecting slow client (queue overflow)");
    disconnect();
    return;
  }

  std::string content = out_buffer_.str();
  out_buffer_.str("");
  out_buffer_.clear();
  queue_for_write(std::move(content));
}

void Session::queue_for_write(std::string content) {
  bool was_empty = write_queue_.empty();
  write_queue_.push_back(std::move(content));
  if (was_empty && !writing_) {
    do_write();
  }
}

void Session::do_write() {
  if (write_queue_.empty()) {
    writing_ = false;
    return;
  }
  writing_ = true;
  auto self = shared_from_this();
  asio::async_write(socket_, asio::buffer(write_queue_.front()),
                    [this, self](asio::error_code ec, std::size_t) {
                      if (ec) {
                        disconnect();
                        return;
                      }
                      write_queue_.pop_front();
                      do_write();  // Continue with next message or stop
                    });
}

void Session::disconnect() {
  if (socket_.is_open()) {
    asio::error_code ec;
    // Errors during disconnect are ignored - socket may already be closed
    (void)socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    (void)socket_.close(ec);
  }
  if (on_disconnect_) {
    on_disconnect_(shared_from_this());
  }
}

std::string Session::pop_input() {
  if (input_queue_.empty()) return "";
  std::string line = std::move(input_queue_.front());
  input_queue_.pop_front();
  return line;
}

// SessionRegistry non-virtual methods
// These write directly to session output buffers.
// Buffers are flushed by Server after command processing.

void SessionRegistry::notify_race(player_t race, const std::string& message) {
  if (update_in_progress()) return;
  for_each_session([&](Session& session) {
    if (session.connected() && session.player() == race) {
      session.out() << message;
    }
  });
}

bool SessionRegistry::notify_player(player_t race, governor_t gov,
                                    const std::string& message) {
  if (update_in_progress()) return false;
  bool delivered = false;
  for_each_session([&](Session& session) {
    if (session.connected() && session.player() == race &&
        session.governor() == gov) {
      session.out() << message;
      delivered = true;
    }
  });
  return delivered;
}
