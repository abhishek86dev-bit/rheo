#include <algorithm>
#include <cstdint>
#include <llvm/ADT/StringRef.h>
#include <rheo/Diagnostics/SourceManager.h>
#include <rheo/Diagnostics/Span.h>
#include <vector>

namespace rheo {

LineColumn SourceFile::getLineCol(BytePos pos) const {
  auto lower_bound = std::ranges::lower_bound(lineStarts, pos);
  std::uint32_t line = 0;
  if (lower_bound != lineStarts.end() && *lower_bound == pos) {
    line = lower_bound - lineStarts.begin();
  } else if (lower_bound != lineStarts.begin()) {
    line = (lower_bound - lineStarts.begin()) - 1;
  }
  std::uint32_t col = pos.getValue() - lineStarts[line].getValue();
  return {.line = line + 1, .col = col + 1};
}

std::vector<BytePos> computeLineStarts(llvm::StringRef source) {
  std::vector<BytePos> lineStarts = {BytePos(0)};
  for (size_t i = 0; i < source.size(); ++i) {
    if (source[i] == '\n') {
      lineStarts.emplace_back(i + 1);
    }
  }
  return lineStarts;
}

FileId SourceManager::addFile(llvm::StringRef name, llvm::StringRef source) {
  auto lineStarts = computeLineStarts(source);
  auto fileId = FileId(files.size());
  files.emplace_back(name, source, lineStarts);
  return fileId;
}

[[nodiscard]] const SourceFile *SourceManager::getFile(FileId fileId) const {
  auto fileIdValue = fileId.getId();
  if (fileIdValue >= files.size()) {
    return nullptr;
  }
  return &files[fileIdValue];
}

} // namespace rheo
