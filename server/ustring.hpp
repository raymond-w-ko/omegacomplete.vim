#pragma once

template <int N>
class ustring
{
public:
    friend class TestCases;

    ustring()
    {
        info_.size = 0;
        info_.which = kUsingBuffer;
    }

    ustring(const std::string& input)
    {
        info_.size = input.size();

        if (info_.size == 0)
        {
            info_.which = kUsingBuffer;
            return;
        }

        if (info_.size <= N)
        {
            memcpy(data_.buffer, &input[0], info_.size);
            info_.which = kUsingBuffer;
        }
        else
        {
            data_.pointer = new char[info_.size];
            memcpy(data_.pointer, &input[0], info_.size);
            info_.which = kUsingPointer;
        }
    }

    ustring(const ustring& other)
    {
        info_.which = other.info_.which;
        info_.size = other.info_.size;

        if (other.info_.which == kUsingBuffer)
        {
            memcpy(data_.buffer, other.data_.buffer, other.info_.size);
        }
        else
        {
            data_.pointer = new char[info_.size];
            memcpy(data_.pointer, other.data_.pointer, other.info_.size);
        }
    }

    ustring& operator=(const ustring& other)
    {
        if (this != &other)
        {
            info_.size = other.info_.size;

            if (other.info_.which == kUsingBuffer)
            {
                memcpy(data_.buffer, other.data_.buffer, other.info_.size);
            }
            else
            {
                if (info_.which == kUsingPointer)
                    delete data_.pointer;

                data_.pointer = new char[info_.size];
                memcpy(data_.pointer, other.data_.pointer, other.info_.size);
            }

            info_.which = other.info_.which;
        }

        return *this;
    }

    ustring& operator=(const std::string& input)
    {
        info_.size = input.size();

        if (info_.size == 0)
        {
            if (info_.which == kUsingPointer)
            {
                delete data_.pointer;
                data_.pointer = NULL;
            }

            info_.which = kUsingBuffer;
            return *this;
        }

        if (info_.size <= N)
        {
            memcpy(data_.buffer, &input[0], info_.size);
            info_.which = kUsingBuffer;
        }
        else
        {
            if (info_.which == kUsingPointer)
                delete data_.pointer;

            data_.pointer = new char[info_.size];
            memcpy(data_.pointer, &input[0], info_.size);
            info_.which = kUsingPointer;
        }

        return *this;
    }

    ~ustring()
    {
        if (info_.size > 0 && info_.which == kUsingPointer)
        {
            delete data_.pointer;
        }
    }

    size_t size() const { return info_.size; }
    size_t length() const { return size(); }
    bool empty() const { return size() == 0; }

    const char& operator[](const unsigned index) const
    {
        if (size() <= N)
            return data_.buffer[index];
        else
            return data_.pointer[index];
    }

    operator std::string() const
    {
        const char* source = info_.size <= N ?
            data_.buffer : data_.pointer;
        return std::string(source, info_.size);
    }

    size_t GetHashCode() const
    {
        size_t hash = 23;
        const char* source = info_.size <= N ?
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

        const char* x = info_.size <= N ?
            data_.buffer : data_.pointer;
        const char* y = other.info_.size <= N ?
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

    bool operator<(const ustring& other) const
    {
        if (this->size() < other.size())
            return true;
        if (this->size() > other.size())
            return false;

        if (this->size() == 0)
            return false;

        return memcmp(&((*this)[0]), &other[0], size()) < 0;
    }

    void clear()
    {
        if (info_.size > 0 && info_.which == kUsingPointer)
        {
            delete data_.pointer;
            data_.pointer = NULL;
            info_.which = kUsingBuffer;
        }

        info_.size = 0;
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
        char buffer[N];
        char* pointer;
    } data_;
};

namespace boost
{
template <int N>
struct hash<ustring<N>>
{
    size_t operator()(const ustring& str) const
    {
        return str.GetHashCode();
    }
};
}
