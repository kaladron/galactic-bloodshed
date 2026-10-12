// SPDX-License-Identifier: Apache-2.0

/// \file session.cc
/// \brief Client session connection and async I/O implementation.

module;

import asio;
import gb.presentation;
import std;

module session;

std::shared_ptr<Session> Session::create(asio::ip::tcp::socket socket,
                                         EntityManager& em,
                                         SessionRegistry& registry) {
  return std::make_shared<Session>(PrivateToken{}, std::move(socket), em,
                                   registry);
}

Session::Session(PrivateToken, asio::ip::tcp::socket socket, EntityManager& em,
                 SessionRegistry& registry)
    : socket_(std::move(socket)), game_obj_(em, registry) {
  game_obj_.set_level(ScopeLevel::LEVEL_UNIV);
  // Log connection (get peer address)
  asio::error_code ec;
  auto endpoint = socket_.remote_endpoint(ec);
  if (!ec) {
    std::println(std::cerr, "NEW CONNECTION from {}",
                 endpoint.address().to_string());
  }
}

void Session::start() {
  do_read();
}

void Session::send(std::string_view message) {
  if (ui_mode() != UiMode::JSON) {
    out_buffer_.append(message);
    return;
  }

  while (!message.empty()) {
    const auto nl_pos = message.find('\n');
    std::string_view line = (nl_pos == std::string_view::npos)
                                ? message
                                : message.substr(0, nl_pos);
    if (!line.empty() && line.back() == '\r') {
      line.remove_suffix(1);
    }

    if (line.starts_with("{\"type\":") && line.ends_with('}')) {
      out_buffer_.append(line);
      out_buffer_.push_back('\n');
    } else {
      out_buffer_.append(GB::presentation::render_json_text(
          GB::presentation::TextViewModel{.message = std::string(line)}));
    }

    if (nl_pos == std::string_view::npos) {
      break;
    }
    message.remove_prefix(nl_pos + 1);
  }
}

std::string Session::process_telnet_sequences(std::string_view raw) {
  static constexpr unsigned char TELNET_IAC = 0xFF;
  static constexpr unsigned char TELNET_SE = 0xF0;
  static constexpr unsigned char TELNET_SB = 0xFA;
  static constexpr unsigned char TELNET_WILL = 0xFB;
  static constexpr unsigned char TELNET_WONT = 0xFC;
  static constexpr unsigned char TELNET_DO = 0xFD;
  static constexpr unsigned char TELNET_DONT = 0xFE;
  static constexpr unsigned char TELNET_OPT_TTYPE = 0x18;
  static constexpr unsigned char TELNET_TTYPE_IS = 0x00;

  if (raw.find(static_cast<char>(TELNET_IAC)) == std::string_view::npos) {
    return std::string(raw);
  }

  std::string clean;
  clean.reserve(raw.size());
  for (std::size_t i = 0; i < raw.size();) {
    const auto byte = static_cast<unsigned char>(raw[i]);
    if (byte != TELNET_IAC) {
      clean.push_back(raw[i]);
      ++i;
      continue;
    }
    if (i + 1 >= raw.size()) {
      break;
    }
    const auto cmd = static_cast<unsigned char>(raw[i + 1]);
    if (cmd == TELNET_IAC) {
      clean.push_back(static_cast<char>(TELNET_IAC));
      i += 2;
      continue;
    }
    if (cmd == TELNET_WILL || cmd == TELNET_WONT || cmd == TELNET_DO ||
        cmd == TELNET_DONT) {
      if (i + 2 < raw.size()) {
        const auto opt = static_cast<unsigned char>(raw[i + 2]);
        if (cmd == TELNET_WILL && opt == TELNET_OPT_TTYPE) {
          queue_for_write(std::string("\xff\xfa\x18\x01\xff\xf0", 6));
        }
      }
      i = std::min(raw.size(), i + 3);
      continue;
    }
    if (cmd == TELNET_SB) {
      std::size_t end = i + 2;
      while (end + 1 < raw.size() &&
             !(static_cast<unsigned char>(raw[end]) == TELNET_IAC &&
               static_cast<unsigned char>(raw[end + 1]) == TELNET_SE)) {
        ++end;
      }
      if (i + 3 < end &&
          static_cast<unsigned char>(raw[i + 2]) == TELNET_OPT_TTYPE &&
          static_cast<unsigned char>(raw[i + 3]) == TELNET_TTYPE_IS) {
        terminal_type_.assign(raw.substr(i + 4, end - (i + 4)));
        terminal_capabilities_ =
            GB::presentation::detect_terminal_capabilities(terminal_type_);
      }
      i = (end + 1 < raw.size()) ? end + 2 : raw.size();
      continue;
    }
    i += 2;
  }
  return clean;
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
          std::println(std::cerr,
                       "Disconnecting flooding client (input queue overflow)");
          disconnect();
          return;
        }

        // Extract line from buffer
        std::istream is(&input_buffer_);
        std::string line;
        std::getline(is, line);
        line = process_telnet_sequences(line);
        // Remove trailing \r if present (Windows clients)
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() > MAX_COMMAND_LEN) {
          std::println(
              std::cerr,
              "Disconnecting client (command exceeds MAX_COMMAND_LEN)");
          disconnect();
          return;
        }
        // Add to input queue for command processing
        if (!line.empty()) {
          input_queue_.push_back(std::move(line));
        }
        // Continue reading
        do_read();
      });
}

bool Session::has_pending_output() const {
  return !out_buffer_.empty();
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
    std::println(std::cerr, "Disconnecting slow client (queue overflow)");
    disconnect();
    return;
  }

  queue_for_write(std::exchange(out_buffer_, {}));
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
  if (disconnected_) return;
  disconnected_ = true;
  if (socket_.is_open()) {
    asio::error_code ec;
    if (has_pending_output()) {
      std::string remaining = std::exchange(out_buffer_, {});
      std::size_t offset = 0;
      while (offset < remaining.size() && !ec) {
        offset += socket_.write_some(
            asio::buffer(remaining.data() + offset, remaining.size() - offset),
            ec);
      }
    }
    // Errors during disconnect are ignored - socket may already be closed
    (void)socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    (void)socket_.close(ec);
  }
}

std::string Session::pop_input() {
  if (input_queue_.empty()) return "";
  std::string line = std::move(input_queue_.front());
  input_queue_.pop_front();
  return line;
}
