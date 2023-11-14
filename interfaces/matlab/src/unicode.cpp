#include <utf8.h>
#include <string>
#include <string_view>

namespace alpaqa::mex {
std::u16string utf8to16(std::string_view in) { return utf8::utf8to16(in); }
std::string utf16to8(std::u16string_view in) { return utf8::utf16to8(in); }
} // namespace alpaqa::mex