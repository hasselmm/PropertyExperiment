#include "aproperty.h"

#include <functional>

static_assert(sizeof(aproperty::Setter<int>) <= sizeof(std::function<void(int)>));
