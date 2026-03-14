#ifndef RHEO_COMMON_H
#define RHEO_COMMON_H

namespace rheo {

template <typename... Ts> struct Overloaded : Ts... {
  using Ts::operator()...;
};

template <typename... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace rheo

#endif // !RHEO_COMMON_H
