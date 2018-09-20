/**
 * @file   SelfOrganizingMapLib/Cartesian.h
 * @date   Aug 30, 2018
 * @author Bernd Doser, HITS gGmbH
 */

#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace pink {

template <class T>
struct Info;

//! n-dimensional cartesian layout for SOM
template <uint8_t dim, typename T>
class Cartesian
{
public:

    typedef T value_type;

    template <typename T2>
    using layout_type = Cartesian<dim, T2>;

    Cartesian()
     : length{0}
    {}

    Cartesian(std::array<uint32_t, dim> length, T const& init_value)
     : length(length),
	   data(std::accumulate(length.begin(), length.end(), 1, std::multiplies<uint32_t>()), init_value)
    {}

    T& get(std::array<uint32_t, dim> position)
    {
    	size_t p = 0;
    	for (uint8_t i = 0; i != dim; ++i) p += position[i] * i;
        return data[p];
    }

    T const& get(std::array<uint32_t, dim> position) const
    {
    	size_t p = 0;
    	for (uint8_t i = 0; i != dim; ++i) p += position[i] * i;
        return data[p];
    }

    std::string info() const
    {
    	return std::string("Cartesian<") + std::to_string(dim) + ", " + Info<T>::name() + ">";
    }

private:

    std::array<uint32_t, dim> length;

    std::vector<T> data;

};

template <>
struct Info<float>
{
	static const std::string name() { return "float"; }
};

template <uint8_t dim, typename T>
struct Info<Cartesian<dim, T>>
{
	static const std::string name() { return std::string("Cartesian<") + std::to_string(dim) + ", " + Info<T>::name() + ">"; }
};

} // namespace pink
