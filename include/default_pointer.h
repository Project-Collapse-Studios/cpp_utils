#pragma once
#include <cassert>
#include <utility>


namespace cutil {

/// This is a class that defines a pointer wrapper, where you need to make sure the pointer underneath points to a valid object.
/// @param T the type we want to hold a pointer of.
///
/// Before creating an object out of this type, you need to call default_pointer<T>::init(constructor_args...). This will create a new object of type T with proper constructor arguments.
/// This will become the global *default* object. It will exist until the last default_pointer<T> gets deconstructed.
/// You can create this class by various means, however there are distinct behaviours:
/// 1. You create it with the T* ptr constructor, this means the class underneath will hold the pointer to the object you have passed. It will not manage memory for you!
/// 2. You create it using the default constructor, this means that underneath this class will point to the global default object created with default_pointer<T>::init(...). Automatic memory management included :)
/// 3. You create it with the copy constructor, this copies the behavior from the object we're copying from.
template<typename T>
class default_pointer {

    inline static T* global_ptr = nullptr;
    inline static unsigned int refcount = 0;

    T* our_pointer = nullptr;

public:

    /// Initialize the default_pointer object of type T
    template<typename... Args>
    static void init(Args&&... args) {
        if (default_pointer<T>::global_ptr == nullptr) {
            default_pointer<T>::global_ptr = new T(std::forward<Args>(args)...);
        }
    }

    default_pointer() {
        assert(default_pointer<T>::global_ptr != nullptr);
        this->our_pointer = default_pointer<T>::global_ptr;
        ++default_pointer<T>::refcount;
    }

    default_pointer(const default_pointer& other) {
        assert(default_pointer<T>::global_ptr != nullptr);
        this->our_pointer = other.our_pointer;
        if (this->our_pointer == default_pointer<T>::global_ptr) {
            ++default_pointer<T>::refcount;
        }
    }

    explicit default_pointer(T* ptr) {
        assert(default_pointer<T>::global_ptr != nullptr);
        this->our_pointer = ptr;
    }

    ~default_pointer() {
        if (this->our_pointer == default_pointer<T>::global_ptr) {
            --default_pointer<T>::refcount;
            if (default_pointer<T>::refcount == 0) {
                delete default_pointer<T>::global_ptr;
            }
        }
    }

    bool operator==(const default_pointer& other) {
        return this->our_pointer == other.our_pointer;
    }

    default_pointer& operator=(const default_pointer& other) {
        if (this->operator==(other)) {
            return *this;
        }

        if (this->our_pointer == default_pointer<T>::global_ptr && other.our_pointer != default_pointer<T>::global_ptr) {
            --default_pointer<T>::refcount;
        }

        this->our_pointer = other.our_pointer;
        return *this;
    }

    T* operator->() {
        return this->our_pointer;
    }

    T const * operator->() const {
        return this->our_pointer;
    }
};


}
