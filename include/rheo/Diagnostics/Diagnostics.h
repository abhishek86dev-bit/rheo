#ifndef RHEO_DIAGNOSTICS_H
#define RHEO_DIAGNOSTICS_H

#include "SourceLocation.h"
#include "rheo/Diagnostics/SourceManager.h"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <optional>
#include <string>
#include <utility>

namespace rheo {

enum class Severity : uint8_t { Error, Warning, Note, Help };

struct Label {
  Span Location;
  FileId File;
  std::optional<std::string> Message;
  bool IsPrimary;

  static Label primary(Span Location, FileId File,
                       std::optional<std::string> Message = std::nullopt) {
    return {Location, File, std::move(Message), true};
  }

  static Label secondary(Span Location, FileId File,
                         std::optional<std::string> Message = std::nullopt) {
    return {Location, File, std::move(Message), false};
  }

private:
  Label(Span Location, FileId File, std::optional<std::string> Message,
        bool IsPrimary)
      : Location(Location), File(File), Message(std::move(Message)),
        IsPrimary(IsPrimary) {}
};

struct Diagnostic {
  Severity Severity;
  std::optional<std::string> Code;
  std::string Message;
  llvm::SmallVector<Label, 4> Labels;
  std::optional<std::string> Help;

  explicit Diagnostic(rheo::Severity Severity) : Severity(Severity) {}

  void setMessage(llvm::StringRef Msg) { Message = Msg; }
  void setCode(llvm::StringRef C) { Code = C; }
  void setHelp(llvm::StringRef H) { Help = H; }
  void addLabel(Label L) { Labels.emplace_back(std::move(L)); }
  void addLabels(llvm::ArrayRef<Label> NewLabels) {
    Labels.append(NewLabels.begin(), NewLabels.end());
  }

  void print(llvm::raw_ostream &Out, const SourceManager &SrcMgr) const;
};
} // namespace rheo

#endif // !RHEO_DIAGNOSTICS_H
