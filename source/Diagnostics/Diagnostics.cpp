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

namespace ansi {
static constexpr const char *Reset = "\033[0m";
static constexpr const char *Bold = "\033[1m";
static constexpr const char *Red = "\033[31m";
static constexpr const char *Yellow = "\033[33m";
static constexpr const char *Blue = "\033[34m";
static constexpr const char *Gray = "\033[90m";
static constexpr const char *Green = "\033[32m";
} // namespace ansi

static const char *severityColor(Severity S) {
  switch (S) {
  case Severity::Error:
    return ansi::Red;
  case Severity::Warning:
    return ansi::Yellow;
  case Severity::Note:
    return ansi::Blue;
  case Severity::Help:
    return ansi::Green;
  }
  return ansi::Reset;
}

void Diagnostic::print(llvm::raw_ostream &Out,
                       const SourceManager &SrcMgr) const {
  using namespace ansi;
  const char *Color = severityColor(Severity);

  Out << Bold << Color << severityAsStr(Severity) << Reset;
  if (Code)
    Out << "[" << *Code << "]";
  Out << ": " << Message << "\n";

  for (const auto &Label : Labels) {
    const SourceFile *File = SrcMgr.getFile(Label.File);
    if (!File)
      continue;

    LineColumn Start = File->getLineCol(Label.Location.getStart());
    llvm::StringRef Src = File->getSource();

    size_t LineBegin =
        Label.Location.getStart() - (static_cast<size_t>(Start.Col) - 1);
    size_t LineEnd = Src.find('\n', LineBegin);
    if (LineEnd == llvm::StringRef::npos)
      LineEnd = Src.size();

    llvm::StringRef LineText = Src.slice(LineBegin, LineEnd);

    std::string StartLine = std::to_string(Start.Line);
    std::string StartCol = std::to_string(Start.Col);

    Out << Gray << "╭─ " << Reset << File->getName() << ":" << StartLine << ":"
        << StartCol << "\n";
    Out << Gray << "│" << Reset << "\n";
    Out << Gray << StartLine << " │ " << Reset << LineText << "\n";

    llvm::StringRef Marker = Label.IsPrimary ? "▔" : "─";
    size_t SquiggleLen = std::max<size_t>(1, Label.Location.len());
    std::string UnderlineStr;
    UnderlineStr.reserve(SquiggleLen * Marker.size());
    for (size_t Index = 0; Index < SquiggleLen; ++Index)
      UnderlineStr += Marker;

    Out << Gray << "│" << Reset << "   " << std::string(Start.Col - 1, ' ')
        << Color << UnderlineStr << Reset << "\n";

    if (Label.Message) {
      Out << Gray << "│" << Reset << "   " << std::string(Start.Col - 1, ' ')
          << Color << *Label.Message << Reset << "\n";
    }

    Out << Gray << "│" << Reset << "\n";
  }

  if (Help) {
    Out << Gray << "╰─ help: " << Reset << Green << *Help << Reset << "\n";
  }
}

} // namespace rheo
