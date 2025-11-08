#pragma once

#include <span>
#include <vector>

inline static std::vector<int> extract_fingerprints(std::span<const std::uint8_t> bmp)
{
  std::vector<int> ids;
  ids.reserve(32 * 2); // optimistic reserve; most sensors are sparse
  for (std::size_t byte_idx = 0; byte_idx < bmp.size(); ++byte_idx)
  {
    std::uint8_t b = bmp[byte_idx];
    while (b)
    {
      // find lowest set bit
      int bit = __builtin_ctz(static_cast<unsigned>(b));
      ids.push_back(static_cast<int>(byte_idx * 8 + bit));
      // clear lowest set bit
      b &= static_cast<std::uint8_t>(b - 1);
    }
  }
  return ids;
}