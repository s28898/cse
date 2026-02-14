//
// Created by Bruno Kuś on 11/02/2026.
//

#ifndef MONGO_2_CONCEPTS_H
#define MONGO_2_CONCEPTS_H

#include <ranges>

template<class T, class R>
concept InputRangeOf = std::ranges::input_range<T> && std::is_same_v<std::ranges::range_value_t<T>, R>;


#endif //MONGO_2_CONCEPTS_H
