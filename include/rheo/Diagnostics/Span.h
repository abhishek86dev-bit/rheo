#ifndef RHEO_SPAN_H
#define RHEO_SPAN_H

#include <algorithm>
#include <cassert>
#include <compare>
#include <cstdint>

namespace rheo {

class BytePos {
  std::uint32_t pos;

public:
  explicit BytePos(std::uint32_t pos) : pos(pos) {}
  [[nodiscard]] std::uint32_t getValue() const { return pos; };

  std::strong_ordering operator<=>(const BytePos &) const = default;
  bool operator==(const BytePos &) const = default;
};

class Span {
  BytePos start;
  BytePos end;

public:
  Span(BytePos start, BytePos end) : start(start), end(end) {
    assert(start <= end && "Span start must be before end");
  }
  [[nodiscard]] const BytePos &getStart() const { return start; }
  [[nodiscard]] const BytePos &getEnd() const { return end; }
  [[nodiscard]] std::uint32_t len() const {
    return end.getValue() - start.getValue();
  }
  [[nodiscard]] Span merge(const Span &other) const {
    return {std::min(start, other.start), std::max(end, other.end)};
  }
};

} // namespace rheo

#endif // RHEO_SPAN_H
