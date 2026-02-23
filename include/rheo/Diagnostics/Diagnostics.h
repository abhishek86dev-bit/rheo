#ifndef RHEO_DIAGNOSTICS_H
#define RHEO_DIAGNOSTICS_H

#include "SourceLocation.h"
#include "rheo/Diagnostics/SourceManager.h"
#include <cassert>
#include <cstdint>
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
  Span span;
  FileId fileId;
  std::optional<std::string> message;
  bool isPrimary;

  static Label primary(Span span, FileId fileId,
                       std::optional<std::string> message = std::nullopt) {
    return {span, fileId, std::move(message), true};
  }

  static Label secondary(Span span, FileId fileId,
                         std::optional<std::string> message = std::nullopt) {
    return {span, fileId, std::move(message), false};
  }

private:
  Label(Span span, FileId fileId, std::optional<std::string> message,
        bool isPrimary)
      : span(span), fileId(fileId), message(std::move(message)),
        isPrimary(isPrimary) {}
};

struct Diagnostic {
  std::string message;
  Severity severity;
  std::optional<std::string> code;
  llvm::SmallVector<Label, 4> labels;
  std::optional<std::string> help;

  explicit Diagnostic(Severity severity) : severity(severity) {}
  void setMessage(llvm::StringRef message) { this->message = message; }
  void setCode(llvm::StringRef code) { this->code = code; }
  void setHelp(llvm::StringRef help) { this->help = help; }
  void addLabel(Label label) { labels.emplace_back(std::move(label)); }
  void addLabels(llvm::ArrayRef<Label> newlabels) {
    labels.append(newlabels.begin(), newlabels.end());
  };
  void print(llvm::raw_ostream &out, const SourceManager &srcMgr) const;
};

} // namespace rheo

#endif // !RHEO_DIAGNOSTICS_H
