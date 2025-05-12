#pragma once
#include <string>
#include <functional> 

struct HashString
{
    size_t hash = 0;
    std::string original;

    HashString() = default;
    HashString(const std::string& str)
    {
        original = str;
        hash     = std::hash<std::string> {}(str);
    }

    bool operator==(const HashString& other) const { return hash == other.hash; }
    bool operator!=(const HashString& other) const { return !(*this == other); }
    bool operator<(const HashString& other) const { return hash < other.hash; }
    const std::string& GetString() const { return original; }
    std::string& GetString() { return original; }
    const bool empty() const { return original.empty(); }
    const char* c_str() const { return original.c_str(); }
    const size_t size() const { return original.size(); }
};

namespace std
{
    template <> struct hash<HashString>
    {
        size_t operator()(const HashString& hs) const { return hash<string> {}(hs.original); }
    };
}