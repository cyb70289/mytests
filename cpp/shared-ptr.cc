#include <iostream>

template <typename T>
class shared_ptr {
private:
    T *ptr_;
    int *refcnt_;

    void inc_ref() {
        ++(*refcnt_);
    }

    void delete_ptr() {
        if (refcnt_ && --(*refcnt_) == 0) {
            std::cout << "delete ptr " << ptr_ << std::endl;
            delete ptr_;
            delete refcnt_;
            ptr_ = nullptr;
            refcnt_ = nullptr;
        }
    }

public:
    shared_ptr(T *ptr = nullptr) : ptr_(ptr), refcnt_(nullptr) {
        if (ptr) {
            refcnt_ = new int{1};
        }
    }

    // copy constructor
    shared_ptr(const shared_ptr &sptr) {
        refcnt_ = sptr.refcnt_;
        ptr_ = sptr.ptr_;
        inc_ref();
    }

    // assignment operator
    shared_ptr& operator=(shared_ptr &sptr) {
        if (this != &sptr) {
            delete_ptr();
            refcnt_ = sptr.refcnt_;
            ptr_ = sptr.ptr_;
            inc_ref();
        }
        return *this;
    }

    virtual ~shared_ptr() {
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

    void free() {
        delete_ptr();
    }
};

int main(void)
{
    int *p = new int;

    shared_ptr<int> s1(p);      // normal constructor
    shared_ptr<int> s2(s1);     // copy constructor
    shared_ptr<int> s3 = s2;    // copy constructor

    shared_ptr<int> s4;
    s4 = s3;                    // assignment operator

    *s1 = 12345678;
    std::cout << *s4 << std::endl;

    return 0;
}
