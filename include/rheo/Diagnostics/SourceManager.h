#ifndef RHEO_SOURCE_MANAGER_H
#define RHEO_SOURCE_MANAGER_H

#include "SourceLocation.h"
#include "llvm/ADT/StringRef.h"
#include <diagnostics.h>
#include <string>
#include <vector>

namespace rheo {

class SourceFile {
  std::string Name;
  std::string Source;
  std::vector<BytePos> LineStarts;

public:
  SourceFile(llvm::StringRef Name, llvm::StringRef Source,
             std::vector<BytePos> LineStarts)
      : Name(Name), Source(Source), LineStarts(std::move(LineStarts)) {}
  [[nodiscard]] llvm::StringRef getName() const { return Name; }
  [[nodiscard]] llvm::StringRef getSource() const { return Source; }
  [[nodiscard]] LineColumn getLineCol(BytePos Pos) const;
};

class SourceManager {
  std::vector<SourceFile> Files;

public:
  FileId addFile(llvm::StringRef Name, llvm::StringRef Source);
  [[nodiscard]] const SourceFile *getFile(FileId FileId) const;
};

} // namespace rheo

#endif // RHEO_SOURCE_MANAGER
