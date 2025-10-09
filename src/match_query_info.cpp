#include "carbio/match_query_info.h"
#if defined(__cpp_lib_format)
#include <format>
#else
#include <sstream>
#endif

namespace carbio
{
std::string to_json(const match_query_info &info) noexcept
{
#if defined(__cpp_lib_format)
  return std::format(R"({{"confidence": {}}})", info.confidence);
#else
  std::ostringstream ss;
  ss << "{ "
     << R"("confidence": )" << info.confidence << " }";
  return ss.str();
#endif
}
} // namespace carbio
