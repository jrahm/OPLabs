#ifndef DELETER_HPP_
#define DELETER_HPP_

/*
 * Author: jrahm
 * created: 2015/02/13
 * Deleter.hpp: <description>
 */

namespace lang {

/**
 * @brief Abstract class that deallocates an object of type T
 * @param T the object type to deallocate
 *
 * This object is useful in a listnener pattern when the observer
 * may need to have a destructor called.
 */
template<class T>
/* abstract */ class Deallocator {
public:
    /**
     * @brief Called when `value` needs to be deallocated
     * @param value the value to deallocate
     */
    virtual void deallocate(T* value) = 0;
};

/**
 * @brief A deallocator that just calls the destructor of the type T and frees the pointer
 * This is a singleton as there is no state that needs to be kept when using this object.
 */
template<class T>
class StandardDeallocator: public Deallocator<T> {
public:

    static StandardDeallocator& instance() {
        static StandardDeallocator* inst;
    
        if ( ! inst )
            inst = new StandardDeallocator();
    
        return *inst;
    }

    virtual void deallocate(T* value) {
        delete value;
    }

private:
    StandardDeallocator() {}
};

template <class T>
StandardDeallocator<T>* std_deallocator() {
    return &StandardDeallocator<T>::instance();
}

/**
 * @brief A dealloctator that does nothing to the value.
 * This is also a singleton since there is no internal state
 * to this class.
 */
template <class T>
class NullDeallocator: public Deallocator<T> {
public:
    static NullDeallocator& instance() {
        static NullDeallocator* inst ;

        if ( ! inst ) inst = new NullDeallocator();
        return *inst;
    }

    virtual void deallocate(T* value) {
        (void) value;
    }

private:
    NullDeallocator() {}
};


}

#endif /* DELETER_HPP_ */
