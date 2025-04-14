#ifndef POOL_HPP
#define POOL_HPP

#include <functional>
#include <memory>
#include <set>

namespace cultimatics {

template <typename T, typename ID = int> class Pool {
public:
  using Handle = std::unique_ptr<T, std::function<void(T *)>>;

  Pool(std::vector<T> &&items) : items{items} {
    for (int i = 0; i < items.size(); ++i) {
      available.insert(i);
    }
  }

  template <typename Call> Handle borrow(ID id, Call onReturn) {
    if (available.erase(id) == 1) {
      return Handle{&items[id], [this, id, onDestroy](T *item) {
                      onReturn(item);
                      this->available.insert(id);
                    }};
    } else {
      return nullptr;
    }
  }

  Handle borrow(ID id) {
    return borrow(id, [](T *) {});
  }

private:
  std::vector<T> items;
  std::set<ID> available;
};
} // namespace cultimatics

#endif