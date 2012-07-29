//
//  heaper.h
//  Argos
//
//  Created by Windoze on 12-7-2.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <algorithm>
#include <functional>
#include <utility>

#ifndef Argos_heaper_h
#define Argos_heaper_h

namespace argos {
    namespace common {
        /**
         * Template heaper builds and operates binary heap on top of container
         */
        template<typename RandomIterator_T, typename Cmp=std::less<typename RandomIterator_T::value_type> >
        struct heaper {
            typedef typename RandomIterator_T::value_type value_type;
            typedef typename RandomIterator_T::difference_type size_type;
            typedef value_type &reference;
            typedef const value_type &const_reference;
            
            /**
             * Constructor
             *
             * @param first first item in the container
             * @param last one-element-after the last item in the container
             * @param min_heap true to build min-root-heap, false to max-root-heap
             */
            inline heaper(RandomIterator_T first, RandomIterator_T last, bool min_heap)
            : size_(0)
            , first_(first)
            , last_(last)
            , desc_(min_heap)
            {}
            
            /**
             * Constructor
             *
             * @param first first item in the container
             * @param last one-element-after the last item in the container
             * @param min_heap true to build min-root-heap, false to max-root-heap
             * @param cmp Comparator instance, use to copy construct owned cmp
             */
            inline heaper(RandomIterator_T first, RandomIterator_T last, bool min_heap, const Cmp &cmp)
            : size_(0)
            , first_(first)
            , last_(last)
            , desc_(min_heap)
            , cmp_(cmp)
            {}

            inline heaper()
            : size_(0)
            {}
            
            /**
             * Returns number of elements in the heap
             */
            inline size_type size() const { return size_; }

            /**
             * Returns number of elements in the heap
             */
            inline size_type &size() { return size_; }
            
            /**
             * Returns max number of elements can be contained by the heap
             */
            inline size_type capacity() const { return last_-first_; }

            /**
             * Returns the top element, which is the smallest one for min-root-heap
             */
            inline reference top() { return *(first_); }

            /**
             * Returns the top element, which is the smallest one for min-root-heap
             */
            inline value_type top() const { return *(first_); }

            /**
             * Returns or sets element at the pos, it may destroy the heap structure
             *
             * If pos>=size, this can be an empty value.
             */
            inline reference operator[](size_type pos) { return at(pos); }            

            /**
             * Returns element at the pos
             *
             * If pos>=size, this can be an empty value, use with caution
             */
            inline const_reference operator[](size_type pos) const { return at(pos); }

            /**
             * Rebuild heap structure after root element changing
             */
            inline void heapify() { siftdown(); }
            
            /**
             * Push element into heap, overflow if needed, heap structure is maintained
             */
            inline void push(const_reference t) {
                if (size_ < capacity()) {
                    size_++;
                    at(size_-1) = t;
                    siftup();
                } else if (size_ > 0 && !less_than(t, top())) {
                    top() = t;
                    heapify();
                }
            }

            /**
             * Push element into heap, overflow to input element if needed, heap structure is maintained
             */
            inline void push(reference t) {
                if (size_ < capacity()) {
                    size_++;
                    std::swap(at(size_-1), t);
                    siftup();
                } else if (size_ > 0 && !less_than(t, top())) {
                    std::swap(top(), t);
                    heapify();
                }
            }
            
            // C++11 only, N2118 rvalue reference, which can optimize large value copy
#if __cplusplus>=201103L
            /**
             * Push element into heap, overflow if needed, heap structure is maintained
             */
            inline void push(value_type &&t) {
                if (size_ < capacity()) {
                    size_++;
                    at(size_-1) = t;
                    siftup();
                } else if (size_ > 0 && !less_than(t, top())) {
                    top() = t;
                    heapify();
                }
            }
#endif
            
            /**
             * Pop top element out of heap, heap structure is maintained
             */
            inline void pop() {
                if (size_ > 0) {
                    std::swap(top(), at(size_-1));
                    size_--;
                    siftdown();                             // adjust heap
                }
            }
            
            
            /**
             * Heap sort last (size-skip) elements
             *
             * After this call, [first+skip, last) should be sorted
             */
            inline void partial_sort(size_type skip) {
                if (skip>=size()) {
                    return;
                }
                size_type sz=size_;
                for (size_type n=0; n<(sz-skip); n++) {
                    pop();
                }
            }
            
        private:
            inline bool less_than(const_reference t1, const_reference t2) const {
                return (!desc_) ^ cmp_(t1, t2);
            }
            
            inline void siftup() {
                size_type i = size_;
                size_type j = i >> 1;
                while (j > 0 && less_than(at(i-1), at(j-1))) {
                    std::swap(at(i-1), at(j-1));
                    i = j;
                    j = j >> 1;
                }
            }

            inline void siftdown() {
                size_type i = 1;
                size_type j = i << 1;               // find smaller child
                size_type k = j + 1;
                if (k <= size_ && less_than(at(k-1), at(j-1))) {
                    j = k;
                }
                while (j <= size_ && less_than(at(j-1), at(i-1))) {
                    std::swap(at(i-1), at(j-1));
                    i = j;
                    j = i << 1;
                    k = j + 1;
                    if (k <= size_ && less_than(at(k-1), at(j-1))) {
                        j = k;
                    }
                }
            }
            
            inline reference at(size_type pos) { return *(first_+pos); }
            inline const_reference at(size_type pos) const { return *(first_+pos); }
            
            size_type size_;
            RandomIterator_T first_;
            RandomIterator_T last_;
            Cmp cmp_;
            bool desc_;
        };

        /**
         * Helper function to create heap on top of container
         *
         * @param c the container to build heap on
         * @param min_heap true to find largest elements, false to find smallest elements
         */
        template<typename Container_T>
        inline heaper<typename Container_T::iterator> make_heap(Container_T &c, bool min_heap=true)
        {
            return heaper<typename Container_T::iterator>(c.begin(), c.end(), min_heap);
        }
    }   // End of namespace common
}   // End of namespace argos

#endif
