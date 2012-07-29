//
//  primes.h
//  Argos
//
//  Created by Windoze on 12-6-12.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stddef.h>

#ifndef Argos_primes_h
#define Argos_primes_h

namespace argos {
    namespace common {
        /**
         * Return a prime number not less than given number
         */
        size_t get_prime_number(size_t n);
    }   // End of namespace common
}   // End of namespace argos
#endif
