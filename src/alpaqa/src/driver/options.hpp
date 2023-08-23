#pragma once

#include <alpaqa/params/params.hpp>

#include <memory>
#include <span>
#include <string_view>
#include <vector>

class Options {
  private:
    std::vector<std::string_view> options_storage;
    std::vector<unsigned> used_storage;

  public:
    Options(int argc, const char *const argv[]) {
        std::copy(argv, argv + argc, std::back_inserter(options_storage));
        used_storage.resize(options_storage.size());
    }
    [[nodiscard]] std::span<const std::string_view> options() const {
        return options_storage;
    }
    [[nodiscard]] std::span<unsigned> used() { return used_storage; }
};

template <class T>
decltype(auto) set_params(T &t, std::string_view prefix, Options &opts) {
    return alpaqa::params::set_params(t, prefix, opts.options(), opts.used());
}
