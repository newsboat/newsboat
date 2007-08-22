#ifndef REFCNT_PTR__H
#define REFCNT_PTR__H

// TODO: replace this with more or less standard implementation, e.g. from boost (if available)

struct deref_exception {
    deref_exception() { }
};

template <class T> class refcnt_ptr {
protected:
    // a helper class that holds the pointer to the managed object
    // and its reference count
    class Holder {
    public:
        Holder( T* ptr) : ptr_(ptr), count_(1) {};
        ~Holder() { if (ptr_) delete ptr_;};

        T* ptr_;
        unsigned count_;
    };

    Holder* h_; 

public:
    // ctor of refcnt_ptr (p must not be NULL)
    explicit refcnt_ptr( T* p) : h_(new Holder(p)) {}
    explicit refcnt_ptr() : h_(new Holder(0)) { }
    // dtor of refcnt_ptr
    ~refcnt_ptr() { if (--h_->count_ == 0) delete h_; }
    // copy and assignment of refcnt_ptr
    refcnt_ptr (const refcnt_ptr<T>& right) : h_(right.h_) { 
        ++h_->count_; 
    }
    refcnt_ptr<T>& operator= (const refcnt_ptr<T>& right) {
        ++right.h_->count_;
        if (--h_->count_ == 0) delete h_;
        h_ = right.h_;
        return *this;
    }
    refcnt_ptr<T>& operator= (refcnt_ptr<T>& right) {
        ++right.h_->count_;
        if (--h_->count_ == 0) delete h_;
        h_ = right.h_;
        return *this;
    }

    bool operator!() {
      return !h_->ptr_;
    }

    bool operator==(const refcnt_ptr<T>& right) {
      return h_->ptr_ == right.h_->ptr_;
    }

    bool operator!=(const refcnt_ptr<T>& right) {
      return !(operator==(right));
    }

    bool operator==(void * ptr) {
        return h_->ptr_ == (T *)ptr;
    }

    bool operator!=(void * ptr) {
        return !(operator==(ptr));
    }

    // access to the managed object
    T* operator-> () const { 
      if (!h_->ptr_) throw deref_exception();
      return h_->ptr_; 
    }

    T& operator* () const { 
      if (!h_->ptr_) throw deref_exception();
      return *h_->ptr_; 
    }

}; 

#endif
