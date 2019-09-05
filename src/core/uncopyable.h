#pragma once

#include "api.h"

class GLRT_API Uncopyable {
public:
    Uncopyable() {}
    virtual ~Uncopyable() {}
    Uncopyable(const Uncopyable &) = delete;
    Uncopyable &operator=(const Uncopyable &) = delete;
};
