#ifndef RHEO_DIAGNOSTIC_ENGINE_H
#define RHEO_DIAGNOSTIC_ENGINE_H

#include "rheo/Diagnostics/Diagnostics.h"
#include <llvm/ADT/ArrayRef.h>
#include <vector>

namespace rheo {

class DiagnosticEngine {
  std::vector<Diagnostic> Diags;

public:
  void emit(const Diagnostic &Diag) { Diags.emplace_back(Diag); };
  [[nodiscard]] llvm::ArrayRef<Diagnostic> diagnostics() const {
    return Diags;
  };
  [[nodiscard]] bool hasError() const { return !Diags.empty(); };
};
} // namespace rheo

#endif
