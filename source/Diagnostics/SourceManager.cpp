#include <llvm/ADT/StringRef.h>
#include <rheo/Diagnostics/Diagnostics.h>
#include <rheo/Diagnostics/SourceManager.h>
#include <vector>

namespace rheo {

LineColumn SourceFile::getLineCol(BytePos Pos) const {
  auto LowerBound = std::ranges::lower_bound(LineStarts, Pos);
  std::uint32_t Line = 0;
  if (LowerBound != LineStarts.end() && *LowerBound == Pos) {
    Line = LowerBound - LineStarts.begin();
  } else if (LowerBound != LineStarts.begin()) {
    Line = (LowerBound - LineStarts.begin()) - 1;
  }
  std::uint32_t Col = Pos - LineStarts[Line];
  return {.Line = Line + 1, .Col = Col + 1};
}

static std::vector<BytePos> computeLineStarts(llvm::StringRef Source) {
  std::vector<BytePos> LineStarts = {BytePos(0)};
  for (size_t I = 0; I < Source.size(); ++I)
    if (Source[I] == '\n')
      LineStarts.emplace_back(I + 1);
  return LineStarts;
}

FileId SourceManager::addFile(llvm::StringRef Name, llvm::StringRef Source) {
  auto LineStarts = computeLineStarts(Source);
  auto Id = FileId(Files.size());
  Files.emplace_back(Name, Source, LineStarts);
  return Id;
}

const SourceFile *SourceManager::getFile(FileId ID) const {
  if (ID >= Files.size()) {
    return nullptr;
  }
  return &Files[ID];
}

} // namespace rheo
