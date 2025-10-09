#include "carbio/search_query_info.h"
#if defined(__cpp_lib_format)
#include <format>
#else
#include <sstream>
#endif

namespace carbio
{
std::string to_json(const search_query_info &info) noexcept
{
#if defined(__cpp_lib_format)
  return std::format(R"({{"index": {}, "confidence": {}}})", info.index, info.confidence);
#else
  std::ostringstream ss;
  ss << "{ "
     << R"("index": )" << info.index << ", "
     << R"("confidence": )" << info.confidence << " }";
  return ss.str();
#endif
}
} // namespace carbio
