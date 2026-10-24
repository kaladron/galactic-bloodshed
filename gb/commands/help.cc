// SPDX-License-Identifier: Apache-2.0

/// \file help.cc
/// \brief Display game documentation and help.

module;

import gb.entities;
import gb.presentation.empire_views;
import gb.services;
import std;

module commands;

namespace GB::commands {

namespace {

std::optional<std::filesystem::path>
resolve_help_path(std::string_view topic, std::string_view help_dir_override) {
  if (topic.empty() || topic.find_first_of("/\\.") != std::string_view::npos) {
    return std::nullopt;
  }
  std::filesystem::path help_dir =
      !help_dir_override.empty() ? std::filesystem::path(help_dir_override)
                                 : std::filesystem::path(HELPDIR);
  auto path = help_dir / std::format("{}.md", topic);
  if (std::filesystem::is_regular_file(path)) {
    return path;
  }
  for (const auto& dev_dir : {"help", "../help", "../../help"}) {
    auto dev_path =
        std::filesystem::path(dev_dir) / std::format("{}.md", topic);
    if (std::filesystem::is_regular_file(dev_path)) {
      return dev_path;
    }
  }
  return std::nullopt;
}

std::optional<std::string> load_help_topic(std::string_view topic,
                                           std::string_view help_dir_override) {
  auto path = resolve_help_path(topic, help_dir_override);
  if (!path) {
    return std::nullopt;
  }
  std::ifstream f(*path);
  std::string content;
  std::string line;
  while (std::getline(f, line)) {
    content += line;
    content.push_back('\n');
  }
  return content;
}

}  // namespace

bool help(const command_t& argv, GameObj& g,
          std::string_view help_dir_override) {
  if (argv.size() > 2) {
    g.out << "Syntax: help [<topic>]\n";
    return false;
  }

  const bool is_index = (argv.size() == 1);
  const std::string_view topic = is_index ? "help" : std::string_view{argv[1]};
  auto content = load_help_topic(topic, help_dir_override);
  if (!content) {
    g.out << (is_index ? "Help file not found.\n"
                       : "Help on that subject unavailable.\n");
    return false;
  }

  g.present(GB::presentation::HelpViewModel{
      .topic = std::string{topic},
      .is_index = is_index,
      .content = *std::move(content),
  });
  return true;
}

bool help(const command_t& argv, GameObj& g) {
  const char* env_help = std::getenv("GB_HELPDIR");
  return help(argv, g,
              env_help ? std::string_view{env_help} : std::string_view{});
}

const CommandDescriptor help_cmd{
    .name = "help",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "help [<topic>]",
    .description = "Display general help or documentation for a specific topic",
    .handler = &help,
};

}  // namespace GB::commands
