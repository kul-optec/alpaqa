#pragma once

#include <alpaqa/params/params.hpp>
#include <alpaqa/params/vec-from-file.hpp>
#include <alpaqa/util/string-util.hpp>

#include <algorithm>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

#if ALPAQA_WITH_JSON
#include <alpaqa/params/json.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#else
#include <alpaqa/util/string-util.hpp>
#endif

class Options {
  private:
    std::vector<std::string_view> opts_storage;
    std::vector<unsigned> used_storage;
    size_t num_json;
#if ALPAQA_WITH_JSON
    std::vector<nlohmann::json> json_storage;
    nlohmann::json json_out;
#endif

  public:
    Options(int argc, const char *const argv[])
        : opts_storage{argv, argv + argc} {
        // Arguments starting with an '@' sign refer to JSON files with options
        auto with_at = [](std::string_view s) { return s.starts_with('@'); };
        // JSON options are always applied before command-line options
        auto non_json = std::ranges::stable_partition(opts_storage, with_at);
        num_json = static_cast<size_t>(non_json.begin() - opts_storage.begin());
#if ALPAQA_WITH_JSON
        // Load the JSON data for each '@' option
        auto load = [](std::string_view s) {
            return load_json(std::string(s.substr(1)));
        };
        auto json_obj = std::views::transform(json_flags(), load);
        json_storage = decltype(json_storage){json_obj.begin(), json_obj.end()};
#else
        if (num_json > 0)
            throw std::logic_error(
                "This version of alpaqa was compiled without JSON support: "
                "cannot parse options " +
                alpaqa::util::join_quote(json_flags(), {.sep = " "}));
#endif
        // Keep track of which options are used
        used_storage.resize(opts_storage.size() - num_json);
    }
    [[nodiscard]] std::span<const std::string_view> json_flags() const {
        return std::span{opts_storage}.first(num_json);
    }
    [[nodiscard]] std::span<const std::string_view> options() const {
        return std::span{opts_storage}.subspan(num_json);
    }
    [[nodiscard]] std::span<unsigned> used() { return used_storage; }

    template <class T>
    void set_params(T &t, std::string_view prefix);

#if ALPAQA_WITH_JSON
    [[nodiscard]] std::span<const nlohmann::json> json_data() const {
        return json_storage;
    }

    [[nodiscard]] const nlohmann::json &get_json_out() const {
        return json_out;
    }

  private:
    [[nodiscard]] static nlohmann::json load_json(const std::string &name) {
        nlohmann::json j;
        std::ifstream f{name};
        if (!f)
            throw std::runtime_error("Unable to open JSON file '" + name + "'");
        try {
            f >> j;
        } catch (nlohmann::json::exception &e) {
            throw std::runtime_error("Unable to parse JSON file '" + name +
                                     "': " + e.what());
        }
        return j;
    }
#endif
};

template <class T>
void Options::set_params(T &t, std::string_view prefix) {
#if ALPAQA_WITH_JSON
    auto json_data = this->json_data();
    for (size_t i = 0; i < json_data.size(); ++i)
        try {
            if (auto j = json_data[i].find(prefix); j != json_data[i].end())
                alpaqa::params::set_param(t, *j);
        } catch (alpaqa::params::invalid_json_param &e) {
            std::string fname{this->json_flags()[i].substr(1)};
            throw std::invalid_argument(
                "Error in JSON file '" + fname + "' at '" +
                std::string(prefix) +
                alpaqa::util::join_quote(std::views::reverse(e.backtrace),
                                         {.sep         = "",
                                          .empty       = "",
                                          .quote_left  = ".",
                                          .quote_right = ""}) +
                "': " + e.what());
        } catch (nlohmann::json::exception &e) {
            std::string fname{this->json_flags()[i].substr(1)};
            throw std::invalid_argument("Error in JSON file '" + fname +
                                        "': " + e.what());
        }
#endif
    alpaqa::params::set_params(t, prefix, this->options(), this->used());
#if ALPAQA_WITH_JSON
    using vec_from_file = alpaqa::params::vec_from_file<alpaqa::DefaultConfig>;
    if constexpr (!std::is_same_v<T, vec_from_file>)
        alpaqa::params::get_param(t, this->json_out[std::string{prefix}]);
#endif
}

template <class T>
void set_params(T &t, std::string_view prefix, Options &opts) {
    opts.set_params(t, prefix);
}
