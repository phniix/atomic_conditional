Atomic Conditional
==================

This is a nice concept that provides the behaviour of a condition variable's
notify all; parlance of pthreads, a broadcast, while in windows land, that's
a wake all or wake by all.

The idea:

 - Some object of type Foo is worked on by a thread
 - We have many Foo object types that are being worked on by indepedent threads
 - Each thread performs the same validation based on the Foo object
 - Some thread ends execution if the Foo object dictates it so
 - The work of one thread ending based on the state of a Foo translates
    to all threads working on Foo to complete in the same fashion.


Using Atomics
-------------

Using atomics provides a smooth way to improve interprocess communication for this
style of notification. While underlying implementations for condition variables on
particular platforms may indeed invoke atomics, more generally, their approach is
to hold onto a mutex.

While holding onto a mutex is alright, notification to all threads is invariably a
loop construct, and suffers with linear response, and can be subject to pre-emption
by the OS scheduler.

The downside to using atomics, is that the entire scheduled time slice is being used
in an effective spin-lock on the atomic condition variable, and not inherent thread
yield occurs if the condition is not met. This is really raw, down to the wire type
of stuff.

### How to provide object level atomic interprocess notification ?

The mechanism for providing the object level notification system is through the
C++ static keyword. Making a static class member variable std::atomic<T> will provide
the primary mechanism.

```cpp

static std::atomic_[type]   _conditional;

```

This is very raw and low level. As we will see. There are approaches for making
this more palatable for users that are averse to dealing with such constructs.


### A guided approach to conditional atomics for Object Types

This is broken into a number of different sections.


#### Code

```cpp

/// \file header.h

class MyClass
{
public:
    /// .. code snip ..

    static std::atomic_bool accessible_;

    /// .. code snip ..
};


/// \file header.cpp

std::atomic_bool MyClass::accessible_ { false };

```

#### Making queries, setting `accessible_`

In a reasonable world, we want to remain sequentially consistent.
Where we want to improve, we remove the reigns, and ask the horse to run free.


*Queries*

In the processing thread's work function, we must check the class static member
variable, to ensure whether the object should be accessible or not:

```cpp

    do_thing( Foo &obj )
    {
        if( Foo::accessible_.load() )
        {
            obj.do_stuff();
        }
    }

```

As each object is running the same code, each thread will synchronise their loads with
any stores across other threads.

In the form written above, we perform the atomic load() operation with sequentially
consistent memory ordering.

*Setting `accessible_`*

In which ever is the controller thread, ie, the one that dictates that for all the Foo
object types its `accessible_` static member variable should be flipped, we perform the
following:

```cpp

    bool finish_work = Foo::accessible_.load();
    while(!Foo::accessible_.compare_exchange_weak(finish_work, true));

```

It is important to note that in the example provided, this is performed from the main
thread, however, this could just as easily be applied from any thread.. hence the
weakly ordered CAS operation. There are alternatives to this notation. One could
perform a hard store,

```cpp

    Foo::accessible_.store(true);

```

An exchange,

```cpp

    Foo::accessible_.exchange(true);

```

Use a strongly typed CAS..

```cpp

    bool finish_work = Foo::accessible_.load();
    while(!Foo::accessible_.compare_exchange_strong(finish_work, true));

```

When we perform our CAS loops, whether they be weak or strong, there is a nice
optimisation we can make.

```cpp

    bool finish_work = Foo::accessible_.load();
    while(!Foo::accessible_.compare_exchange_weak(finish_work, true)
          && !finish_work);

```

It is entirely possible that another thread may well have changed the value of
finish_work to that which we desire, true. Without the !finish_work, we would
performing another loop before trying to write true again to finish_work.




#### Funky Queries with relaxed Memory Ordering

It's time to rock on :D


Synchronize your loads and your stores.





Compiling
---------

### Windows 10, Visual Studio 2017 15 Win64

Enable vcvars.bat amd64

cl /EHsc main.cpp


### Windows Ubuntu g++ 5....

g++ -std=c++14 -Wall -Werror -pedantic -O3 -lpthread main.cpp -o lnxwin_main


### Linux g++ 5.4.1

g++ -std=c++14 -Wall -Werror -pedantic -O3 -lpthread main.cpp -o lnxwin_main


### Linux g++ 6.3.0

g++ -std=c++14 -Wall -Werror -pedantic -O3 -lpthread main.cpp -o lnxwin_main


### Cmake :D

Out of source build:


#### Windows
```
    cmake .. -G "Visual Studio 2017 15 Win64" -DCMAKE_INSTALL_PREFIX=./install
    cmake --build . --target all_build --config Release
    cmake --build . --target install --config Release
```

#### \*nix
```
    cmake .. -DCMAKE_INSTALL_PREFIX=./install -DCMAKE_BUILD_TYPE=Release
    make
    make install
```


Results
-------

Once the concept is firmed up and solidifed, it's nice to explore and test your
approaches. This concept is situational, and it is important to see how it fits
in with what you are writing and what you are writing for.

This may seem neat, but it may not totally apply.
You should consult your architecture.


### Darwin - 2 cores, 3 threads - OSX 10.11, clang 8.xxxx



### Linux - 8 cores, 3 threads - Ubuntu 16.04 4.4.0, g++ 6.30


#### Assembly

    http://godbolt.org


### Linux - 8 cores, 3 threads - Ubuntu 16.04 4.4.0, g++ 5.4.1


#### Assembly

    http://godbolt.org


### Windows - 8 cores, 3 threads - MSVC 15 Win64


### Windows - 8 cores, 3 threads - Ubuntu ....., g++ v????

#### Assembly

    http://godbolt.org










