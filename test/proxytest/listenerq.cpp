
#include <functional>
#include <atomic>
#include <thread>
#include <latch>
#include <chrono>

#include <iostream>
#include <ostream>

#include <cstdio>

#include <smrproxy.h>

// #include <stdio.h>


/**
 * Example of using smr_ref.update_epoch to implement a multi-headed queue as an event listener.
*/

template<typename T>
    requires std::is_trivially_copyable_v<T> && std::is_default_constructible_v<T>
struct xnode : public smr_obj_base
{
    std::atomic<xnode*> next = nullptr;
    std::latch latch;

    T event_data;
    // ....
    xnode() : latch(1) {}

    ~xnode()
    {
        std::fprintf(stdout, "     node dtor %p event_data=%d pre_expiry=%lu expiry=%lu\n",
                            this, event_data,
                            pre_expiry.load(std::memory_order_relaxed),
                            expiry.load(std::memory_order_relaxed));
        event_data = -2;
        next.store(nullptr);
    }
};

template<typename T>
struct queue_t {
    std::atomic<xnode<T>*> tail = new xnode<T>(); // event queue
    // std::mutex mutex;
    // std::condition_variable_any cvar;


    queue_t() {}
    ~queue_t()
    {
        xnode<T>* _tail = tail.exchange(nullptr);
        _tail->event_data = -99;
        std::fprintf(stdout, "     queue dtor delete tail %p event_data=%d\n", _tail, _tail->event_data);
        delete _tail;
    }

    /**
     * enqueue new event onto listener queue
     * 
     * @param ref local smrproxy shared ref
     * @param event_data event id for new event
     */
    void enqueue(smrproxy& proxy, smr_ref& ref, T event_data)
    {
        xnode<T>* node = new xnode<T>();

        { std::scoped_lock lock(ref);

            xnode<T>* prev = tail.exchange(node, std::memory_order_acquire);
            prev->next.store(node, std::memory_order_release);  //?
            prev->event_data = event_data;
            prev->latch.count_down();   // make available to listeners

            proxy.retire(prev);         // release event once all listeners received it.
        }
    }

    // void listen(smrproxy& proxy, std::latch& start, bool (*listener)(xnode* event))
    void listen(smrproxy* proxy, const std::function<bool (xnode<T>*)>& listener, std::latch* start = nullptr)
    {
        smr_ref* ref = proxy->acquire_ref();

        {
            std::scoped_lock m(*ref);

            xnode<T>* node = tail.load(std::memory_order_acquire);

            if (start != nullptr)
                start->count_down();

            for (;;)
            {
                node->latch.wait();     // wait for event to fire

                if (!listener(node))
                    break;

                ref->update_epoch(node);
                node = node->next.load(std::memory_order_acquire);
            }

        }   // ref lock

        proxy->release_ref(ref);
    }

};



/*
 * listener reader thread
*/
static void listen(const int id, const int sleep, std::latch* start, smrproxy* proxy, queue_t<int>* queue)
{

    auto delay_ms = std::chrono::milliseconds(sleep);

    auto listener =  [id, delay_ms] (xnode<int>* event) {
        std::fprintf(stdout, "(%02d) node=%p event_data=%d pre_expiry=%lu expiry=%lu\n", id, event, 
                            event->event_data,
                            event->pre_expiry.load(std::memory_order_relaxed),
                            event->expiry.load(std::memory_order_relaxed));

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));


        return event->event_data != -1;
    };
    queue->listen(proxy, listener, start);

}


int main(int argc, char **argv)
{
    std::latch start(2);    // 2 listener threads

    smrproxy proxy(10);
    queue_t<int> queue;

    std::thread listener1(listen, 1, 30, &start, &proxy, &queue);
    std::thread listener2(listen, 2, 40, &start, &proxy, &queue);

    start.wait();

    smr_ref* ref = proxy.acquire_ref();

    for (int ndx = 1; ndx <= 20; ndx++)
    {
        queue.enqueue(proxy, *ref, 1000 + ndx);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));

    }
    queue.enqueue(proxy, *ref, -1);
    std::fprintf(stdout, "==== end of event input ====\n");

    listener1.join();
    listener2.join();

    proxy.release_ref(ref);



    return 0;
}