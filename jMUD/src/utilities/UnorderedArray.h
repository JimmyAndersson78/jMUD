#ifndef UNORDEREDARRAY_H
#define UNORDEREDARRAY_H

#include <cstddef>
#include <cassert>



//
// Supports insertion (at unspecified location), removal at specified location, retrieval from specified
// location, capacity specification and size/emptyness querying.
//
template<typename T, std::size_t N> class UnorderedArray {

  public:
#ifdef DEBUG
    UnorderedArray(void) : _size(0), _peak(0) {}
    std::size_t peak(void) {return _peak;}
#else
    UnorderedArray(void) : _size(0) {}
#endif
    ~UnorderedArray() {;}

    bool push(T e);
    T    pop(void);
    void remove(std::size_t index);
    T    at(std::size_t index);

    bool empty(void);
    bool full(void);
    std::size_t size(void);
    std::size_t capacity(void);

  private:
    UnorderedArray(const UnorderedArray&);
    UnorderedArray& operator=(const UnorderedArray&);

    std::size_t _size;
#ifdef DEBUG
    std::size_t _peak;
#endif
    T _array[N];
};


template <class T, std::size_t N> inline bool UnorderedArray<T, N>::push(T e) {
    if (_size == N)
        return false;
    _array[_size++] = e;

#ifdef DEBUG
    if (_size > _peak)
        _peak = _size;
#endif
    return true;
}

template <class T, std::size_t N> inline T UnorderedArray<T, N>::pop(void) {
    assert(_size > 0);
    // FIXME: Implement handling if the array is empty.
    //    if (empty())
    //        throw
    return _array[--_size];
}


template <class T, std::size_t N> inline void UnorderedArray<T, N>::remove(std::size_t index) {
    assert(index < _size);
    // FIXME: Implement handling of access of undefined/unallocated elements.
    //    if (index >= _size)
    //        throw

    // NOTE: If the element removed wasn't the only element and isn't the last element move the last element
    //       to the removed elements position.
    // TODO: Check how much of an effect the condition has vs just running the swap all the time... since
    //       it will never be "wrong" just unnecessary if the condition is true.
    if (index != --_size) {
        _array[index] = _array[_size];
    }
}


template <class T, std::size_t N> inline T UnorderedArray<T, N>::at(std::size_t index) {
    assert(index < _size);
// FIXME: Implement handling of access of undefined/unallocated elements.
//    if (index >= _size)
//        throw
    return _array[index];
}


template <class T, std::size_t N> inline bool UnorderedArray<T, N>::empty(void) {
    if (_size == 0)
        return true;
    return false;
}


template <class T, std::size_t N> inline bool UnorderedArray<T, N>::full(void) {
    if (_size == N)
        return true;
    return false;
}


template <class T, std::size_t N> inline std::size_t UnorderedArray<T, N>::size(void) {
    return _size;
}


template <class T, std::size_t N> inline std::size_t UnorderedArray<T, N>::capacity(void) {
    return N;
}

#endif // UNORDEREDARRAY_H
