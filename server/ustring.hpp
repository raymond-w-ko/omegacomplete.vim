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
            info_.which = kUsingBuffer;
        }
        else
        {
            data_.pointer = new char[info_.size];
            ::memcpy(data_.pointer, input, info_.size);
            info_.which = kUsingPointer;
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
            info_.which = kUsingBuffer;
        }
        else
        {
            data_.pointer = new char[info_.size];
            ::memcpy(data_.pointer, &input[0], info_.size);
            info_.which = kUsingPointer;
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
            data_.pointer = new char[info_.size];
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

    bool operator==(const ustring& other) const
    {
        if (info_.size != other.info_.size)
            return false;
        if (info_.which != other.info_.which)
            return false;

        const char* x = info_.size <= sizeof(void*) ?
            data_.buffer : data_.pointer;
        const char* y = other.info_.size <= sizeof(void*) ?
            other.data_.buffer : other.data_.pointer;

        for (size_t i = 0; i < info_.size; ++i)
            if (x[i] != y[i])
                return false;

        return true;
    }

    bool operator!=(const ustring& other) const
    {
        return !(*this == other);
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
