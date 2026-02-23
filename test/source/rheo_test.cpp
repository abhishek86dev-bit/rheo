#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "rheo" ? 0 : 1;
}
