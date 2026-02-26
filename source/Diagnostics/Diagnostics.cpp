#include "llvm/ADT/StringRef.h"
#include <llvm/Support/ErrorHandling.h>
#include <rheo/Diagnostics/Diagnostics.h>

namespace rheo {

static inline llvm::StringRef severityAsStr(Severity Serverity) {
  switch (Serverity) {
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

void Diagnostic::print(llvm::raw_ostream &Out,
                       const SourceManager &SrcMgr) const {
  Out << severityAsStr(Severity);
  if (Code) {
    Out << "[" << *Code << "]";
  }
  Out << ": " << Message << "\n";
  for (const auto &Label : Labels) {
    const SourceFile *File = SrcMgr.getFile(Label.File);
    if (File == nullptr) {
      continue;
    }
    LineColumn Start = File->getLineCol(Label.Location.getStart());
    LineColumn End = File->getLineCol(Label.Location.getEnd());
    Out << (Label.IsPrimary ? "  --> " : "  ... ") << File->getName() << ":"
        << std::to_string(Start.Line) << ":" << std::to_string(Start.Col);
    if (Start.Line == End.Line) {
      Out << "-" << std::to_string(End.Col);
    }
    if (Label.Message) {
      Out << ": " << *Label.Message;
    }
    Out << "\n";
    llvm::StringRef Src = File->getSource();
    size_t LineBegin = Label.Location.getStart() - (Start.Col - 1);
    size_t LineEnd = Src.find('\n', LineBegin);
    llvm::StringRef LineText = Src.slice(LineBegin, LineEnd);
    Out << "   | " << LineText << "\n";
    Out << "   | ";
    size_t SquiggleLen =
        static_cast<size_t>(std::max(1U, Label.Location.len()));
    char Marker = Label.IsPrimary ? '^' : '-';
    Out << std::string(static_cast<size_t>(Start.Col - 1), ' ')
        << std::string(SquiggleLen, Marker) << "\n";
  }
  if (Help) {
    Out << "  help: " << *Help << "\n";
  }
}

} // namespace rheo
