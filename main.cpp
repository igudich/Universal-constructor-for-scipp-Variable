#include <iostream>
#include <type_traits>
#include <vector>
#include <memory>
#include <optional>
#include <tuple>
#include <iostream>

struct Unit {
  int unit;
  void print() const {
    std::cout << "Unit: " << unit << "\n";
  }
};

struct Shape {
  std::vector<int> shape;
  void print() const {
    std::cout << "Shape: [";
    for (auto&& e: shape)
      std::cout << e << ", ";
    std::cout << " ]\n";
  }
};

struct HolderInterface{
  virtual void print() const = 0;
};

template <class T>
struct Holder : public HolderInterface {
  std::vector<T> array;
  Holder () = default;
  explicit Holder (std::vector<T>&& data) : array{data} {}
  void print() const override {
    std::cout << "Holder: [ ";
    for(auto&& e: array)
      std::cout << e << ", ";
    std::cout << " ]\n";
  }
};

template <class T>
struct Values : public Holder<T> {
  Values(const Values&) = delete;
  explicit Values(std::vector<T>&& v) : Holder<T>(std::move(v)) {};
  template <class...Ts> explicit Values(Ts&&...args) : Holder<T>(std::forward<Ts>(args)...) {};
};
template<class T> Values(std::vector<T>&&) -> Values<T>;

template <class T>
struct Variances : Holder<T> {
  Variances(const Variances&) = delete;
  explicit Variances(std::vector<T>&& v) : Holder<T>(std::move(v)) {};
  template <class...Ts> explicit Variances(Ts&&...args) : Holder<T>(std::forward<Ts>(args)...) {};
};
template<class T> Variances(std::vector<T>&&) -> Variances<T>;

struct Variable {
  Unit unit;
  std::vector<int> shape;
  std::unique_ptr<HolderInterface> values;
  std::optional<std::unique_ptr<HolderInterface>> variances;
  template <class T>
  Variable(Unit&& unit, Shape&& shape, Values<T>&& values, Variances<T>&& variances) :
    unit{unit}, shape(shape.shape), values(std::make_unique<Values<T>>(values)), variances(std::make_unique<Variances<T>>(variances)) {
      if (variances.array.empty())
        this->variances = std::nullopt;
    }

  void print() const {
    unit.print();
    Shape{shape}.print();
    values->print();
    if (variances)
      variances.value()->print();
    else
      std::cout << "No variances\n";
    std::cout << "\n";
  }
};

template <class T, class ...Args>
using hasType = std::disjunction<std::is_same<T, std::decay_t<Args>>...>;

template <class...Ts> //given types
struct helper {
  template <class...Args> //needed types
  constexpr static int check_types() {
    return (hasType<Args, Ts...>::value + ...);
  }

  template <class T, class...Args>
  static T construct(Ts&&...ts) {
    std::cout << "Types: ";
    ((std::cout << ',' << std::is_rvalue_reference<decltype(std::forward<Ts>(ts))>::value), ...);
    std::cout << "\n";
    auto tp = std::make_tuple(std::forward<Ts>(ts)...);
    auto tpFallBack = std::tuple(Args{}...);
    return T(std::forward<Args>(hlp<Args, decltype(tpFallBack), Ts...>(tp, tpFallBack))...);
  }
  private:
    template<class T, class FBTuple, class...Args>
    static T& hlp(std::tuple<Args...>& tp, FBTuple& fb) {
      if constexpr (hasType<T, Ts...>::value)
        return std::get<T>(tp);
      else
        return std::get<T>(fb);
    }
};


template <class T, class...Ts>
Variable makeVariable(Ts&&...args) {
  using helper = helper<Ts...>;
  constexpr auto nn = helper::template check_types<Unit, Shape, Values<T>, Variances<T>>();
  static_assert(nn == std::tuple_size_v<decltype(std::forward_as_tuple(args...))>);
  return helper::template construct<Variable, Unit, Shape, Values<T>, Variances<T>>(std::forward<Ts>(args)...);
}

int main() {
  makeVariable<double>(Shape{{1}}, Unit{1}).print();
  makeVariable<double>(Shape{{1}}).print();
  makeVariable<double>(Shape{{1,23, }}, Values<double>{std::vector<double>{1,4,5}}, Unit{1}).print();
  makeVariable<double>(Shape{{1}}).print();
  makeVariable<double>(Shape{{1,2,3 }}, Variances<double>{std::vector<double>{6,7,8}}, Unit{23}).print();
  makeVariable<double>().print();
  makeVariable<float>(Values{std::vector{1.0f,2.0f,3.0f}}).print();
  makeVariable<double>(Unit{3}).print();
  makeVariable<double>(Values<double>{{1,4,5}}, Unit{1}).print();
}
