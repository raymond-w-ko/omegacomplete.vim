#pragma once

class Buffer
{
public:
    Buffer();
    ~Buffer();
    bool Init(std::string pathname);
    
    bool operator==(const Buffer& other);
    std::string GetName() const;
    
    void Parse();

private:
    std::string name_;
    std::string contents_;
};

namespace std
{
template<> struct hash<Buffer>
{
    size_t operator()(const Buffer& buffer) const
    {
        return std::hash<std::string>()(buffer.GetName());
    }
};
}
