#ifndef UNORDEREDQUEUEMT_H
#define UNORDEREDQUEUEMT_H

#include <stack>    // std::stack<T>
#include <mutex>    // std::mutex



// A thread-safe "queue" of unordered messages. Perfect for temporarily holding non-ordered messages to be
// passed between systems.
// TODO: Implement using a more optimal underlying data structure.
// TODO: Generalize and cleanup the implementation, possibly make it STL compatible if possible/reasonable.
// TODO: Add some prefix to the class name to indicate that the container is really only intended for
//       storing pointers (and possibly other basic data types).
//       A: How about just prefixing with a P?
template<typename T, std::size_t N = 0> class UnorderedQueueMT {
  public:
    UnorderedQueueMT() : _stack(), _mutex() {}
    ~UnorderedQueueMT() {}

    void push(T e);
    T    pop(void);
    T    front(void);

    bool empty(void);
    bool full(void);
    std::size_t size(void);

    void lock(void);
    void unlock(void);

  private:
    UnorderedQueueMT(const UnorderedQueueMT&);
    UnorderedQueueMT& operator=(const UnorderedQueueMT&);

    std::stack<T> _stack;
    std::mutex    _mutex;
};

template <class T, std::size_t N> inline void UnorderedQueueMT<T, N>::push(T e) {
    _stack.push(e);
}

template <class T, std::size_t N> inline T UnorderedQueueMT<T, N>::pop(void) {
    T e = _stack.top();
    _stack.pop();
    return e;
}

template <class T, std::size_t N> inline T UnorderedQueueMT<T, N>::front(void) {
    return _stack.top();
}

template <class T, std::size_t N> inline bool UnorderedQueueMT<T, N>::empty(void) {
    return _stack.empty();
}

template <class T, std::size_t N> inline bool UnorderedQueueMT<T, N>::full(void) {
    return false;
}

template <class T, std::size_t N> inline std::size_t UnorderedQueueMT<T, N>::size(void) {
    return _stack.size();
}

template <class T, std::size_t N> inline void UnorderedQueueMT<T, N>::lock(void) {
    _mutex.lock();
}

template <class T, std::size_t N> inline void UnorderedQueueMT<T, N>::unlock(void) {
    _mutex.unlock();
}

#endif // UNORDEREDQUEUEMT_H
