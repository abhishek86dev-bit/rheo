#ifndef RHEO_SYMBOL_H
#define RHEO_SYMBOL_H

#include "rheo/Diagnostics/SourceLocation.h"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>

namespace rheo {

struct SymbolId {
public:
  constexpr SymbolId() : Id(0) {}
  explicit constexpr SymbolId(uint32_t Id) : Id(Id) {}

  static constexpr SymbolId invalid() { return SymbolId(0); }

  bool isValid() const { return Id != 0; }

  uint32_t get() const { return Id; }

  explicit operator bool() const { return isValid(); }

  bool operator==(SymbolId Other) const { return Id == Other.Id; }
  bool operator!=(SymbolId Other) const { return Id != Other.Id; }

private:
  uint32_t Id;
};

struct Scope {
  SymbolId get(llvm::StringRef Name) {
    auto It = Symbols.find(Name);
    if (It != Symbols.end())
      return It->second;
    return SymbolId::invalid();
  }

  void insert(llvm::StringRef Name, SymbolId Id) { Symbols[Name] = Id; }

  bool contains(llvm::StringRef Name) { return Symbols.contains(Name); }

private:
  llvm::SmallDenseMap<llvm::StringRef, SymbolId, 8> Symbols;
};

using Scopes = llvm::SmallVector<Scope, 8>;

enum class SymbolKind {
  Function,
  Param,
  Variable,
};

struct Symbol {
  llvm::StringRef Name;
  Span Location;
  bool IsMutable;
  SymbolKind Kind;

  static Symbol function(llvm::StringRef Name, Span Location) {
    return Symbol(Name, Location, false, SymbolKind::Function);
  }

  static Symbol param(llvm::StringRef Name, Span Location) {
    return Symbol(Name, Location, false, SymbolKind::Param);
  }

  static Symbol variable(llvm::StringRef Name, Span Location,
                         bool IsMutable = false) {
    return Symbol(Name, Location, IsMutable, SymbolKind::Variable);
  }

private:
  Symbol(llvm::StringRef Name, Span Location, bool IsMutable, SymbolKind Kind)
      : Name(Name), Location(Location), IsMutable(IsMutable), Kind(Kind) {}
};

struct SymbolTable {
  SymbolId insert(const Symbol &S) {
    Symbols.push_back(S);
    return SymbolId(Symbols.size()); // IDs start from 1
  }

  Symbol &get(SymbolId Id) { return Symbols[Id.get() - 1]; }

  const Symbol &get(SymbolId Id) const { return Symbols[Id.get() - 1]; }

  size_t size() const { return Symbols.size(); }

  void reserve(size_t N) { Symbols.reserve(N); }

private:
  llvm::SmallVector<Symbol, 8> Symbols;
};

} // namespace rheo

#endif // !RHEO_SYMBOL_H
