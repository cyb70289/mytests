#include <iostream>

template <typename T>
class uniq_ptr {
private:
    T *ptr_;

    void delete_ptr() {
        if (ptr_)
            std::cout << "delete ptr " << ptr_ << std::endl;
        delete ptr_;
    }

public:
    uniq_ptr(T *ptr = nullptr) : ptr_(ptr) {
    }

    // move constructor
    uniq_ptr(uniq_ptr &&uptr) {
        ptr_ = uptr.ptr_;
        uptr.ptr_ = nullptr;
    }

    // assignment operator
#if 1
    uniq_ptr& operator=(uniq_ptr &uptr) {
        if (this != &uptr) {
            delete_ptr();
            ptr_ = uptr.ptr_;
            uptr.ptr_ = nullptr;
        }
        return *this;
    }
#else
    uniq_ptr& operator=(uniq_ptr uptr) {
        std::swap(uptr.ptr_, ptr_);
        return *this;
    }
#endif

    virtual ~uniq_ptr() {
        delete_ptr();
    }

    T& operator*() const {
        return *ptr_;
    }

    T* operator->() const {
        return ptr_;
    }

    T* get() {
        return ptr_;
    }
};

int main(void)
{
    int *p = new int;
    int *q = new int;

    uniq_ptr<int> up(p);
    std::cout << "1. up.ptr_ = " << up.get() << std::endl;

    uniq_ptr<int> up2(std::move(up));
    std::cout << "2. up.ptr_ = " << up.get() << std::endl;
    std::cout << "3. up2.ptr_ = " << up2.get() << std::endl;

    uniq_ptr<int> uq(q);
    std::cout << "4. uq.ptr_ = " << uq.get() << std::endl;

    uq = up2;
    std::cout << "5. up2.ptr_ = " << up2.get() << std::endl;
    std::cout << "6. uq.ptr_ = " << uq.get() << std::endl;

    return 0;
}
