# ifndef ___OPTIONAL_H___
# define ___OPTIONAL_H___

#include <boost/optional.hpp>

template<typename T>
using OPT = boost::optional<T>;

const auto NULLOPT = boost::none;

# endif //___OPTIONAL_HPP___
