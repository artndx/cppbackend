#pragma once

#include <boost/program_options.hpp>
#include <optional>
#include <vector>
#include <iostream>

namespace cmd_parser{

struct Args {
    std::optional<double> tick_period;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]);

} // namespace cmd_parser