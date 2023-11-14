#pragma once

#include <cstring>
#include <functional>
#include <streambuf>
#include <string_view>
#include <vector>

namespace alpaqa::mex {

/// Output stream buffer that calls the given function with the characters.
/// UTF-8 aware.
/// Based on the implementation I wrote for pybind11
/// @see https://github.com/pybind/pybind11/commit/0c93a0f3fcf6bf26be584558d7426564720cea6f
/// @see https://github.com/pybind/pybind11/blob/e250155afadde7100e627e6aa4a541137a863243/include/pybind11/iostream.h
class streambuf : public std::streambuf {
  public:
    using write_utf8_func_t = std::function<void(std::string_view)>;

  private:
    using traits_type = std::streambuf::traits_type;

    const size_t buf_size;
    std::vector<char> d_buffer;
    write_utf8_func_t write_utf8;

    int overflow(int c) override {
        if (!traits_type::eq_int_type(c, traits_type::eof())) {
            *pptr() = traits_type::to_char_type(c);
            pbump(1);
        }
        return sync() == 0 ? traits_type::not_eof(c) : traits_type::eof();
    }

    // Computes how many bytes at the end of the buffer are part of an
    // incomplete sequence of UTF-8 bytes.
    // Precondition: pbase() < pptr()
    [[nodiscard]] size_t utf8_remainder() const {
        const auto rbase = std::reverse_iterator<char *>(pbase());
        const auto rpptr = std::reverse_iterator<char *>(pptr());
        auto is_ascii    = [](char c) {
            return (static_cast<unsigned char>(c) & 0x80) == 0x00;
        };
        auto is_leading = [](char c) {
            return (static_cast<unsigned char>(c) & 0xC0) == 0xC0;
        };
        auto is_leading_2b = [](char c) {
            return static_cast<unsigned char>(c) <= 0xDF;
        };
        auto is_leading_3b = [](char c) {
            return static_cast<unsigned char>(c) <= 0xEF;
        };
        // If the last character is ASCII, there are no incomplete code points
        if (is_ascii(*rpptr)) {
            return 0;
        }
        // Otherwise, work back from the end of the buffer and find the first
        // UTF-8 leading byte
        const auto rpend   = rbase - rpptr >= 3 ? rpptr + 3 : rbase;
        const auto leading = std::find_if(rpptr, rpend, is_leading);
        if (leading == rbase) {
            return 0;
        }
        const auto dist  = static_cast<size_t>(leading - rpptr);
        size_t remainder = 0;

        if (dist == 0) {
            remainder = 1; // 1-byte code point is impossible
        } else if (dist == 1) {
            remainder = is_leading_2b(*leading) ? 0 : dist + 1;
        } else if (dist == 2) {
            remainder = is_leading_3b(*leading) ? 0 : dist + 1;
        }
        // else if (dist >= 3), at least 4 bytes before encountering an UTF-8
        // leading byte, either no remainder or invalid UTF-8.
        // Invalid UTF-8 will cause an exception later when converting
        // to a Python string, so that's not handled here.
        return remainder;
    }

    // This function must be non-virtual to be called in a destructor.
    int _sync() {
        if (pbase() != pptr()) { // If buffer is not empty
            // This subtraction cannot be negative, so dropping the sign.
            auto size        = static_cast<size_t>(pptr() - pbase());
            size_t remainder = utf8_remainder();

            if (size > remainder) {
                std::string_view line(pbase(), size - remainder);
                write_utf8(line);
            }

            // Copy the remainder at the end of the buffer to the beginning:
            if (remainder > 0) {
                std::memmove(pbase(), pptr() - remainder, remainder);
            }
            setp(pbase(), epptr());
            pbump(static_cast<int>(remainder));
        }
        return 0;
    }

    int sync() override { return _sync(); }

  public:
    explicit streambuf(write_utf8_func_t write_utf8, size_t buffer_size = 1024)
        : buf_size(buffer_size), d_buffer(buf_size),
          write_utf8(std::move(write_utf8)) {
        setp(d_buffer.data(), d_buffer.data() + buf_size - 1);
    }

    streambuf(streambuf &&) = default;

    // Sync before destroy
    ~streambuf() override { _sync(); }
};

} // namespace alpaqa::mex