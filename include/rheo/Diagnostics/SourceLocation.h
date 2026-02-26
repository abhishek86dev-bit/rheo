#ifndef RHEO_SOURCE_LOCATION_H
#define RHEO_SOURCE_LOCATION_H

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace rheo {

using FileId = std::uint32_t;
using BytePos = std::uint32_t;

class Span {
  BytePos Start;
  BytePos End;

public:
  Span(BytePos Start, BytePos End) : Start(Start), End(End) {
    assert(Start <= End && "Span start must be before end");
  }
  [[nodiscard]] const BytePos &getStart() const { return Start; }
  [[nodiscard]] const BytePos &getEnd() const { return End; }
  [[nodiscard]] std::uint32_t len() const { return End - Start; }
  [[nodiscard]] Span merge(const Span &Other) const {
    return {std::min(Start, Other.Start), std::max(End, Other.End)};
  }
};

struct LineColumn {
  std::uint32_t Line;
  std::uint32_t Col;
};

} // namespace rheo

#endif // RHEO_SOURCE_LOCATION_H
