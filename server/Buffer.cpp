#include "stdafx.hpp"

#include "Buffer.hpp"

Buffer::Buffer()
{
    ;
}

Buffer::~Buffer()
{
    ;
}

bool Buffer::operator==(const Buffer& other)
{
    return (this->name_ == other.name_);
}

void Buffer::Parse()
{
    ;
}

bool Buffer::Init(std::string pathname)
{
    if (pathname.size() == 0) throw std::exception();
    
    name_ = pathname;
}

std::string Buffer::GetName() const
{
    return name_;
}
