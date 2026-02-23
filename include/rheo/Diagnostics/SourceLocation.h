#ifndef RHEO_SOURCE_LOCATION_H
#define RHEO_SOURCE_LOCATION_H

#include <algorithm>
#include <cassert>
#include <compare>
#include <cstdint>

namespace rheo {

class FileId {
  std::uint32_t fileId;

public:
  explicit FileId(std::uint32_t fileId) : fileId(fileId) {}
  [[nodiscard]] std::uint32_t getId() const { return fileId; }
  std::strong_ordering operator<=>(const FileId &) const = default;
  bool operator==(const FileId &) const = default;
};

class BytePos {
  std::uint32_t pos;

public:
  explicit BytePos(std::uint32_t pos) : pos(pos) {}
  [[nodiscard]] std::uint32_t getValue() const { return pos; }
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

struct LineColumn {
  std::uint32_t line;
  std::uint32_t col;
};

} // namespace rheo

#endif // RHEO_SOURCE_LOCATION_H
