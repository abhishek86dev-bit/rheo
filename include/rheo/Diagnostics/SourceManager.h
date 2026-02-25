#ifndef RHEO_SOURCE_MANAGER_H
#define RHEO_SOURCE_MANAGER_H

#include "SourceLocation.h"
#include "llvm/ADT/StringRef.h"
#include <diagnostics.h>
#include <string>
#include <vector>

namespace rheo {

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
