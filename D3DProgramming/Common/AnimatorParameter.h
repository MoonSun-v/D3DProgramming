#pragma once
#include <unordered_map>
#include <string>

class AnimatorParameters
{
public:
    void SetBool(const std::string& name, bool v) { bools[name] = v; }
    bool GetBool(const std::string& name) const
    {
        auto it = bools.find(name);
        return it != bools.end() ? it->second : false;
    }

    void SetFloat(const std::string& name, float v) { floats[name] = v; }
    float GetFloat(const std::string& name) const
    {
        auto it = floats.find(name);
        return it != floats.end() ? it->second : 0.0f;
    }

private:
    std::unordered_map<std::string, bool>  bools;
    std::unordered_map<std::string, float> floats;
};