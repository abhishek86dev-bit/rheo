#ifndef RHEO_DIAGNOSTIC_ENGINE_H
#define RHEO_DIAGNOSTIC_ENGINE_H

#include "rheo/Diagnostics/Diagnostics.h"
#include <llvm/ADT/ArrayRef.h>
#include <vector>

namespace rheo {

class DiagnosticEngine {
  std::vector<Diagnostic> diags;

public:
  void emit(const Diagnostic &diag) { diags.emplace_back(diag); };
  [[nodiscard]] llvm::ArrayRef<Diagnostic> diagnostics() const {
    return diags;
  };
  [[nodiscard]] bool hasError() const { return !diags.empty(); };
};
} // namespace rheo

#endif
