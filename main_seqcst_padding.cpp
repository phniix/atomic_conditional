#include <atomic>
#include <thread>
#include <functional>   // std::ref

#include <iostream>


class Foo;
void do_thing(Foo& obj);


class Foo
{
public:
    Foo(int i)
        : m_thread{ &do_thing, std::ref(*this) }
    {
        m_v.store(i);
    }

    ~Foo()
    {
        if(m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void do_work()
    {
        // Just ask 1 to be added at some point in time...
        m_v.fetch_add(1);
    }

    inline int count() const
    {
        // Load acquire, synchronises with
        return m_v.load();
    }

    static std::atomic_bool m_cond;

private:
    char pad1[64];
    std::atomic_int m_v { 0 };
    char pad[64];
    std::thread     m_thread;
};


///
/// \brief This worker function performs operations on the Foo object.
///
/// Foo's class level static atomic, m_cond, acts as a condition variable
/// for any Foo object that is created. This worker function uses the condition
/// variable to stop doing work.
///
///
void do_thing(Foo &obj)
{
    while(!Foo::m_cond.load())
    {
        obj.do_work();
    }
}

std::atomic_bool Foo::m_cond { false };


int main(int argc, const char *argv[])
{
    auto short_sleep = [](){
            std::cout << "Sleeping for 100us..\n";
            std::this_thread::sleep_for( std::chrono::microseconds(100) );
            std::cout << "Resuming..\n";
        };

    Foo a { 1 };
    Foo b { 2 };
    Foo c { 3 };

    auto tpc_count = [&a, &b, &c](int &ac, int &bc, int &cc){
            ac = a.count();
            bc = b.count();
            cc = c.count();

            std::cout << ac << "<<< Timepoint Count (a)\n"
                      << bc << "<<< Timepoint Count (b)\n"
                      << cc << "<<< Timepoint Count (c)\n";
        };

    short_sleep();

    int ckp1_a, ckp1_b, ckp1_c;
    tpc_count(ckp1_a, ckp1_b, ckp1_c);

    // Check the class level "condition variable".
    int spurious { 0 };
    while(!Foo::m_cond.load())
    {
        int cnt = a.count();
        if(cnt > 1)
        {
            std::cout << "Signal reached, Count (a): " << cnt << "\n";

            bool finish_work = Foo::m_cond.load();
            while(!Foo::m_cond.compare_exchange_weak(finish_work,
                                                     true)
		&& !finish_work )
            {
                ++spurious;
            }
        }
    }

    std::cout << "==> Atomic Conditional Set (spurious failure count: "
              << spurious << ") \n";

    int ckp2_a, ckp2_b, ckp2_c;
    tpc_count(ckp2_a, ckp2_b, ckp2_c);

    // Do some additional work on (a)
    std::cout << "==> Do additional work on (a)\n";
    a.do_work();

    short_sleep();

    int ckp3_a, ckp3_b, ckp3_c;
    tpc_count(ckp3_a, ckp3_b, ckp3_c);

    std::cout << "(a): " << std::boolalpha << (ckp3_a == (ckp2_a+1)) << "\n"
              << "(b): " << (ckp3_b == ckp2_b) << "\n"
              << "(c): " << (ckp3_c == ckp2_c) << "\n";


    return 0;

}
