#pragma once

#include <cassert>
#include <string>
#include <concepts>
#include <algorithm>

struct iSerial
{
    std::string in_buffer;
    
    iSerial() = default;
    iSerial(std::string str) : in_buffer{str} {}
    
    template <typename T>
    requires std::integral<T> or std::floating_point<T>
    iSerial& operator >> (T& num)
    {
        auto data = in_buffer.substr(0, sizeof(num));
                    in_buffer.erase(0, sizeof(num));
        
        if constexpr (std::endian::native == std::endian::little)
            std::reverse(data.begin(), data.end());
        
        num = *reinterpret_cast<decltype(&num)>(data.data());
        return *this;
    }
};

struct oSerial
{
    std::string out_buffer;
    
    oSerial() = default;
    oSerial(std::string str) : out_buffer{str} {}
    
    template <typename T>
    requires std::integral<T> or std::floating_point<T>
    oSerial& operator << (const T num)
    {
        std::string data((char*)&num, (char*)(&num + 1));
        
        if constexpr (std::endian::native == std::endian::little)
            std::reverse(data.begin(), data.end());
        
        out_buffer += data; return *this;
    }
};
