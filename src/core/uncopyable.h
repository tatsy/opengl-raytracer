#pragma once

class Uncopyable {
public:
    Uncopyable() {}
    virtual ~Uncopyable() {}
    Uncopyable(const Uncopyable &) = delete;
    Uncopyable &operator=(const Uncopyable &) = delete;
};
