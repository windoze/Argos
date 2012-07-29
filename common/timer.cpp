//
//  timer.cpp
//  Argos
//
//  Created by Windoze on 12-7-23.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <sys/time.h>
#if __cplusplus>=201103L
#include <atomic>
#endif
#include <boost/thread/thread.hpp>

namespace argos {
    namespace common {
        namespace detail {
            /**
             * Too many places in the program need to get current time, gettimeofday/localtime/... are all syscalls
             * and will cause a user/kernel switch.
             * We create an individual thread keep fetching current time and store into a array, then other threads
             * can get current without syscall
             */
            struct timer_thread {
                static double timer[2];
#if __cplusplus>=201103L
                static std::atomic<int> current_timer;
#else
                static volatile int current_timer;
#endif
                
                static volatile double get_time() {
#if __cplusplus>=201103L
                    int ct;
                    std::atomic_load<int>(&current_timer, &ct);
                    return timer[ct];
#else
                    return timer[current_timer];
#endif
                }
                
                void operator()() {
                    struct timeval tv;
                    while (1) {
                        if (gettimeofday(&tv, NULL)==0) {
                            double t=double(tv.tv_sec) + double(tv.tv_usec)/1000000;
#if __cplusplus>=201103L
                            int ct;
                            std::atomic_load<int>(&current_timer, &ct);
                            ct = ct ^ 1;
                            timer[ct]=t;
                            std::atomic_save<int>(&current_timer, &ct);
#else
                            timer[current_timer ^ 1]=t;
                            current_timer = current_timer ^ 1;
#endif
                        }
                        // Just yield will cause high CPU load, although this is not a real problem, but it can
                        // mess every performance counter and monitoring tool.
                        //boost::this_thread::yield();
                        // Sleep 0.1ms can keep CPU load under 5% with enough precision
                        struct timespec ts;
                        ts.tv_sec=0;
                        ts.tv_nsec=100000;  // 0.1ms=100000nanosec
                        nanosleep(&ts, NULL);
                    }
                }
            };
            
            double timer_thread::timer[2]={0,0};
#if __cplusplus>=201103L
            std::atomic<int> timer_thread::current_timer=0;
#else
            volatile int timer_thread::current_timer=0;
#endif

            struct timer_starter {
                timer_starter()
                : t(timer_thread())
                {
                    // This is a daemon thread and we will never join it
                    t.detach();
                }
                
                boost::thread t;
            } the_timer_starter;
        }   // End of namespace detail
        
        volatile double get_time() {
            return detail::timer_thread::get_time();
        }
    }   // End of namespace common
}   // End of namespace argos
