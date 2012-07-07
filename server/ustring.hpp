#pragma once

class ustring
{
public:
    friend class TestCases;

    ustring()
    {
        info_.size = 0;
        info_.which = kUsingBuffer;
    }

    ustring(const char* input)
    {
        if (input == NULL)
            return;

        info_.size = ::strlen(input);
        if (info_.size <= sizeof(void*))
        {
            ::memcpy(data_.buffer, input, info_.size);
        }
        else
        {
            data_.pointer = new char[info_.size];
            ::memcpy(data_.pointer, input, info_.size);
        }
    }

    ustring(const std::string& input)
    {
        info_.size = input.size();

        if (info_.size == 0)
            return;

        if (info_.size <= sizeof(void*))
        {
            ::memcpy(data_.buffer, &input[0], info_.size);
        }
        else
        {
            data_.pointer = new char[info_.size];
            ::memcpy(data_.pointer, &input[0], info_.size);
        }
    }

    ustring(const ustring& other)
    {
        this->info_.which = other.info_.which;
        this->info_.size = other.info_.size;

        if (other.info_.which == kUsingBuffer)
        {
            ::memcpy(data_.buffer, other.data_.buffer, other.info_.size);
        }
        else
        {
            ::memcpy(data_.pointer, other.data_.pointer, other.info_.size);
        }
    }

    ~ustring()
    {
        if (info_.which == kUsingPointer)
            delete data_.pointer;
    }

    size_t size() const { return info_.size; }
    size_t length() const { return size(); }

    char& operator[](const unsigned index)
    {
        if (size() <= sizeof(void*))
            return data_.buffer[index];
        else
            return data_.pointer[index];
    }

    operator std::string() const
    {
        const char* source = info_.size <= sizeof(void*) ?
            data_.buffer : data_.pointer;
        return std::string(source, info_.size);
    }

    size_t GetHashCode() const
    {
        size_t hash = 23;
        const char* source = info_.size <= sizeof(void*) ?
            data_.buffer : data_.pointer;
        for (size_t i = 0; i < info_.size; ++i)
        {
            hash = hash * 37 + static_cast<size_t>( source[i] );
        }
        return hash;
    }

private:
    enum RepresentationType
    {
        kUsingBuffer = 0,
        kUsingPointer = 1,
    };

    struct
    {
        unsigned size : 31;
        unsigned which : 1;
    } info_;

    union
    {
        char buffer[sizeof(void*)];
        char* pointer;
    } data_;
};

namespace boost
{
template<> struct hash<ustring>
{
    size_t operator()(const ustring& str) const
    {
        return str.GetHashCode();
    }
};
}
