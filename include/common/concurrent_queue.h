//
//  concurrent_queue.h
//  Argos
//
//  Created by Windoze on 12-7-22.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <queue>
#include <algorithm>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#ifndef Argos_concurrent_queue_h
#define Argos_concurrent_queue_h

namespace argos {
    namespace common {
        template<typename T, typename Container = std::deque<T> >
        struct concurrent_queue {
            typedef concurrent_queue<T, Container> this_type;
            typedef std::queue<T, Container> queue_type;
            typedef boost::mutex mutex_type;

            typedef typename queue_type::container_type container_type;
            typedef typename queue_type::value_type value_type;
            typedef typename queue_type::size_type size_type;
            typedef typename queue_type::reference reference;
            typedef typename queue_type::const_reference const_reference;
            
            inline explicit concurrent_queue(size_type capacity=size_type(-1), bool auto_open=true)
            : opened_(auto_open)
            , capacity_(capacity)
            {}
            
            inline bool open() {
                mutex_type::scoped_lock lock(the_mutex_);
                opened_=true;
            }
            
            inline void close() {
                mutex_type::scoped_lock lock(the_mutex_);
                opened_=false;
                full_cv_.notify_all();
                empty_cv_.notify_all();
            }
            
            inline bool closed() const {
                mutex_type::scoped_lock lock(the_mutex_);
                return opened_;
            }
            
            inline bool push(const T &data) {
                mutex_type::scoped_lock lock(the_mutex_);
                if (!opened_) {
                    // Cannot push into a closed queue
                    return false;
                }
                // Wait until queue is closed or not full
                while((the_queue_.size()>=capacity_) && opened_)
                {
                    full_cv_.wait(lock);
                }
                if (!opened_) {
                    // Queue closed
                    return false;
                }
                the_queue_.push(data);
                empty_cv_.notify_one();
                return true;
            }
            
            inline bool try_push(const T &data) {
                mutex_type::scoped_lock lock(the_mutex_);
                if (!opened_) {
                    // Cannot push into a closed queue
                    return false;
                }
                // Check if the queue is full
                if(the_queue_.size()>=capacity_)
                {
                    return false;
                }
                the_queue_.push(data);
                empty_cv_.notify_one();
                return true;
            }

            template<typename InIterator>
            inline size_type push_n(InIterator ii, size_type nelem) {
                mutex_type::scoped_lock lock(the_mutex_);
                if (!opened_) {
                    // Cannot push into a closed queue
                    return false;
                }
                // Check if the queue is full
                if(the_queue_.size()>=capacity_)
                {
                    return false;
                }
                nelem=std::min(capacity_-the_queue_.size(), nelem);
                size_type i=0;
                for(; i<nelem; i++) {
                    the_queue_.push(*ii);
                    empty_cv_.notify_one();
                }
                return i;
            }
            
#if __cplusplus>=201103L
            inline bool push(T &&data) {
                mutex_type::scoped_lock lock(the_mutex_);
                if (!opened_) {
                    // Cannot push into a closed queue
                    return false;
                }
                // Wait until queue is closed or not full
                while((the_queue_.size()>=capacity_) && opened_)
                {
                    full_cv_.wait(lock);
                }
                if (!opened_) {
                    // Queue closed
                    return false;
                }
                the_queue_.push(data);
                empty_cv_.notify_one();
                return true;
            }
            
            inline bool try_push(T &&data) {
                mutex_type::scoped_lock lock(the_mutex_);
                if (!opened_) {
                    // Cannot push into a closed queue
                    return false;
                }
                // Check if the queue is full
                if(the_queue_.size()>=capacity_)
                {
                    return false;
                }
                the_queue_.push(data);
                empty_cv_.notify_one();
                return true;
            }
#endif
            inline bool pop(T &popped_value) {
                mutex_type::scoped_lock lock(the_mutex_);
                // Wait only if the queue is open and empty
                while(the_queue_.empty() && opened_)
                {
                    empty_cv_.wait(lock);
                }
                if (the_queue_.empty()) {
                    // Last loop ensure queue will not empty only if queue is closed
                    // So here we have an empty and closed queue
                    return false;
                }
                std::swap(popped_value, the_queue_.front());
                the_queue_.pop();
                full_cv_.notify_one();
                return true;
            }
            
            inline bool try_pop(T& popped_value)
            {
                mutex_type::scoped_lock lock(the_mutex_);
                if(the_queue_.empty())
                {
                    return false;
                }
                std::swap(popped_value, the_queue_.front());
                the_queue_.pop();
                full_cv_.notify_one();
                return true;
            }

            template<typename OutIterator>
            inline size_type pop_n(OutIterator oi, size_type nelem)
            {
                mutex_type::scoped_lock lock(the_mutex_);
                if(the_queue_.empty())
                {
                    return false;
                }
                nelem=std::min(the_queue_.size(), nelem);
                size_type i=0;
                for(; i<nelem; i++) {
                    std::swap(*oi, the_queue_.top());
                    the_queue_.pop();
                    oi++;
                    full_cv_.notify_one();
                }
                return i;
            }
            
            inline bool empty() const {
                mutex_type::scoped_lock lock(the_mutex_);
                return the_queue_.empty();
            }
            
            inline bool full() const {
                mutex_type::scoped_lock lock(the_mutex_);
                return the_queue_.size() >= capacity_;
            }
            
            inline size_type size() const {
                mutex_type::scoped_lock lock(the_mutex_);
                return the_queue_.size();
            }
            
            inline size_type capacity() const {
                return capacity_;
            }
            
        private:
            // Non-copyable
            inline concurrent_queue(const concurrent_queue &){};
            inline void operator=(const concurrent_queue &){};

            bool opened_;
            const size_t capacity_;
            mutable mutex_type the_mutex_;
            boost::condition_variable full_cv_;
            boost::condition_variable empty_cv_;
            queue_type the_queue_;
        };
    }   // End of namespace common
}   // End of namespace argos

#endif
