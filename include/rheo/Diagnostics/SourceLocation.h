#ifndef RHEO_SOURCE_LOCATION_H
#define RHEO_SOURCE_LOCATION_H

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace rheo {

using FileId = std::uint32_t;
using BytePos = std::uint32_t;

class Span {
  BytePos start;
  BytePos end;

public:
  Span(BytePos start, BytePos end) : start(start), end(end) {
    assert(start <= end && "Span start must be before end");
  }
  [[nodiscard]] const BytePos &getStart() const { return start; }
  [[nodiscard]] const BytePos &getEnd() const { return end; }
  [[nodiscard]] std::uint32_t len() const { return end - start; }
  [[nodiscard]] Span merge(const Span &other) const {
    return {std::min(start, other.start), std::max(end, other.end)};
  }
};

struct LineColumn {
  std::uint32_t line;
  std::uint32_t col;
};

} // namespace rheo

#endif // RHEO_SOURCE_LOCATION_H
