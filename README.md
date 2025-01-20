# Proxies
Various proxy implementations, i.e. synchronous and deferred memory reclamation implemenations,
for read lock-free access, in C++.

1. smrproxy - wait-free proxy
2. arcproxy - lock-free reference counted proxy
3. rwlock based proxy - for comparison
4. mutex based proxy - for comparison
5. no-op based proxy - unsafe read acess

## Proxy methods
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

## Tests and performance tests
The main performance testing program is in
test/proxy/test

```
$ ./proxytest -h
Options:
  -c --count <arg>  reader access count (default 0)
  -n --nreaders <arg>  number of reader threads (default 0)
  -m --mod <arg>  reader sleep on every n'th access (default 0)
  -r --rsleep_ms <arg>  reader sleep duration in milliseconds (default 0)
  -w --wsleep_ms <arg>  writer sleep duration in milliseconds (default 0)
     --reclaim_ms <arg> reclaim poll interval in milliseconds (default 50)
     --no_stats no data state statistics (default false)
  -s --size <arg> size of arcproxy (default 512)
  -t --type testcase:
    smr -- smrproxy
    arc -- arcproxy
    rwlock -- rwlock based proxy
    mutex -- mutex based proxy
    unsafe -- unsafe access
    all -- all tests w/ summaries only
    all2 -- unsafe, smr, smrlite w/ summaries only
  -v --verbose show config values (default false)
  -q --quiet less output (default false)
```

### Output examples
```
$ ./proxytest -c 1000000 -n 8 -t all
test         cpu time      (unsafe diff)     elapsed time  (unsafe diff)
unsafe         9.243 nsecs (      0.000)      15.422 nsecs (      0.000)
smr            8.312 nsecs (      0.000)      11.195 nsecs (      0.000)
arc          269.965 nsecs (    260.722)     298.093 nsecs (    282.671)
rwlock       927.557 nsecs (    918.314)     974.961 nsecs (    959.539)
mutex      1,054.002 nsecs (  1,044.759)   1,156.847 nsecs (  1,141.425)

$ ./proxytest -c 100000000 -n 8 -t smr
testcase: smr
Statistics:
  reader thread count = 8
  read_count = 800,000,000
  elapsed cpu read_time = 6,547,644,872 nsecs
  avg cpu read_time =     8.185 nsecs
  elapsed read_time = 831,057,405 nsecs
  avg elapsed read time =      8.311 nsecs
  data state counts:
    live = 800,000,000
    stale = 0
    invalid = 0
    other = 0
  retire_count = 1
  elapsed retire_time = 136 nsecs
  avg retire_time =     0.136 usecs
  data allocs = 1
  data deletes = 1
  voluntary context switches = 15
  involuntary context switches = 124
  user cpu time = 6,548,130,000 nsecs
  system cpu time = 0 nsecs
```
