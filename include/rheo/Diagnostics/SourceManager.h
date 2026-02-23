#ifndef RHEO_SOURCE_MANAGER
#define RHEO_SOURCE_MANAGER

#include "Span.h"
#include "llvm/ADT/StringRef.h"
#include <compare>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace rheo {

class FileId {
  std::uint32_t fileId;

public:
  explicit FileId(std::uint32_t fileId) : fileId(fileId) {}
  [[nodiscard]] std::uint32_t getId() const { return fileId; };

  std::strong_ordering operator<=>(const FileId &) const = default;
  bool operator==(const FileId &) const = default;
};

struct LineColumn {
  std::uint32_t line;
  std::uint32_t col;
};

class SourceFile {
  std::string name;
  std::string source;
  std::vector<BytePos> lineStarts;

public:
  SourceFile(llvm::StringRef name, llvm::StringRef source,
             std::vector<BytePos> lineStarts)
      : name(name), source(source), lineStarts(std::move(lineStarts)) {}
  [[nodiscard]] llvm::StringRef getName() const { return name; }
  [[nodiscard]] llvm::StringRef getSource() const { return source; }
  [[nodiscard]] LineColumn getLineCol(BytePos pos) const;
};

class SourceManager {
  std::vector<SourceFile> files;

public:
  FileId addFile(llvm::StringRef name, llvm::StringRef source);
  [[nodiscard]] const SourceFile *getFile(FileId fileId) const;
};

} // namespace rheo

#endif // RHEO_SOURCE_MANAGER
