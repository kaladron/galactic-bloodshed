// SPDX-License-Identifier: Apache-2.0

/// \file help.cc
/// \brief Display game documentation and help.

module;

import gb.entities;
import gb.services;
import std;

module commands;

namespace GB::commands {

static std::optional<std::filesystem::path>
resolve_help_path(std::string_view topic) {
  if (topic.empty() || topic.find_first_of("/\\.") != std::string_view::npos) {
    return std::nullopt;
  }
  const char* env_help = std::getenv("GB_HELPDIR");
  std::filesystem::path help_dir = (env_help && *env_help != '\0')
                                       ? std::filesystem::path(env_help)
                                       : std::filesystem::path(HELPDIR);
  auto path = help_dir / std::format("{}.md", topic);
  if (std::filesystem::exists(path)) {
    return path;
  }
  for (const auto& dev_dir : {"help", "../help", "../../help"}) {
    auto dev_path =
        std::filesystem::path(dev_dir) / std::format("{}.md", topic);
    if (std::filesystem::exists(dev_path)) {
      return dev_path;
    }
  }
  return std::nullopt;
}

static bool print_help_file(const std::filesystem::path& path, GameObj& g) {
  auto f = std::ifstream(path);
  if (!f) {
    return false;
  }
  std::string line;
  while (std::getline(f, line)) {
    g.out << line << "\n";
  }
  return true;
}

bool help(const command_t& argv, GameObj& g) {
  if (argv.size() == 1) {
    auto path = resolve_help_path("help");
    if (path && print_help_file(*path, g)) {
      return true;
    }
    g.out << "Help file not found.\n";
    return false;
  }

  auto path = resolve_help_path(argv[1]);
  if (path && print_help_file(*path, g)) {
    g.out << "----\nFinished.\n";
    return true;
  }
  g.out << "Help on that subject unavailable.\n";
  return false;
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
