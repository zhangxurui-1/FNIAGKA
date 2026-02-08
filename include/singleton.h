#pragma once

#include <memory>
template <typename T>

class Singleton {
    Singleton() = delete;
    ~Singleton() = delete;
    std::shared_ptr<T> GetInstance() {
        if (instance_ == nullptr) {
            instance_ = std::make_shared<T>();
        }
        return instance_;
    }

  private:
    static std::shared_ptr<T> instance_;
};
