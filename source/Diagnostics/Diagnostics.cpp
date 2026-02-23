#include "llvm/ADT/StringRef.h"
#include <llvm/Support/ErrorHandling.h>
#include <rheo/Diagnostics/Diagnostics.h>

namespace rheo {

inline llvm::StringRef severityAsStr(Severity serverity) {
  switch (serverity) {
  case Severity::Error:
    return "error";
  case Severity::Warning:
    return "warning";
  case Severity::Note:
    return "note";
  case Severity::Help:
    return "help";
  }
  llvm_unreachable("Unknown severity");
}

// this only for debugging
void Diagnostic::print(llvm::raw_ostream &out,
                       const SourceManager &srcMgr) const {
  out << severityAsStr(severity);
  if (code) {
    out << "[" << *code << "]";
  }
  out << ": " << message << "\n";
  for (const auto &label : labels) {
    const SourceFile *file = srcMgr.getFile(label.fileId);
    if (file == nullptr) {
      continue;
    }
    LineColumn start = file->getLineCol(label.span.getStart());
    LineColumn end = file->getLineCol(label.span.getEnd());
    out << (label.isPrimary ? "  --> " : "  ... ") << file->getName() << ":"
        << std::to_string(start.line) << ":" << std::to_string(start.col);
    if (start.line == end.line) {
      out << "-" << std::to_string(end.col);
    }
    if (label.message) {
      out << ": " << *label.message;
    }
    out << "\n";
    llvm::StringRef src = file->getSource();
    size_t lineBegin = label.span.getStart().getValue() - (start.col - 1);
    size_t lineEnd = src.find('\n', lineBegin);
    llvm::StringRef lineText = src.slice(lineBegin, lineEnd);
    out << "   | " << lineText << "\n";
    out << "   | ";
    size_t squiggleLen = static_cast<size_t>(std::max(1U, label.span.len()));
    char marker = label.isPrimary ? '^' : '-';
    out << std::string(static_cast<size_t>(start.col - 1), ' ')
        << std::string(squiggleLen, marker) << "\n";
  }
  if (help) {
    out << "  help: " << *help << "\n";
  }
}

} // namespace rheo
