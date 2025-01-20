# Proxies
Various proxy implementations, i.e. synchronous and deferred memory reclamation implemenations,
for read lock-free access, in C++.

1. smrproxy - wait-free proxy
2. arcproxy - lock-free reference counted proxy
3. rwlock based proxy - for comparison
4. mutex based proxy - for comparison
5. no-op based proxy - unsafe read acess

# Proxy methods
The C++ concept for proxies

```
/**
 * @tparm T proxy collector type
 * @tparm R shared lock reference
 * @tparm B base object type for managed objects
 */
template<typename T, typename R, typename B>
concept ProxyType = requires(T proxy, R* ref, B * obj) 
{
    BasicLockable<R>;
    std::has_virtual_destructor<B>::value;                  // base object must have virtual destructor

    { proxy.acquire_ref() } -> std::same_as<R*>;            // acquire shared lock reference
    { proxy.release_ref(ref) } -> std::same_as<void>;       // release shared lock reference
    { proxy.retire(obj) } -> std::same_as<void>;            // reclaim object when it is no longer referenced
};
```

The shared lock references are BasicLockable and are thread local.
These should be cached in a thread_local variable to avoid the
overhead of acquiring and releasing them for every read locked
access.

Each proxy implemenation has a base object class w/ a virtual
destructor.  Managed objects that are candidates for deferred
reclamation should extend these base classes.

# Tests and performance tests
