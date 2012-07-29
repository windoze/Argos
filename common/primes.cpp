//
//  primes.cpp
//  Argos
//
//  Created by Windoze on 12-7-5.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stddef.h>

namespace argos {
    namespace common {
        namespace detail {
            const size_t  g_prime_numbers[] = {
                17, 47, 97, 193, 389, 787, 1579, 3163, 6329, 12659, 25321, 50647,
                100003, 200003, 300007, 400009, 500009,
                600011, 700001, 800011, 900001, 1000003,
                2000003, 3000017, 4000037, 5000011, 6000011,
                7000003, 8000009, 9000011,
                10000019, 11000027, 12000017, 13000027, 14000029,
                15000017, 16000057, 17000023, 18000041, 19000013,
                20000003, 21000037, 22000001, 23000009, 24000001,
                25000009, 26000003, 27000011, 28000019, 29000039,
                30000001, 31000003, 32000011, 33000001, 34000009,
                35000011, 36000007, 37000039, 38000009, 39000001,
                40000003, 50000017, 60000011, 70000027, 80000023,
                90000049,
                100000007,
                110000017,
                120000007,
                130000027,
                140000041,
                150000001,
                160000003,
                170000009,
                180000017,
                190000003,
                200000033,
                300000007,
                400000009,
                500000003,
                600000001,
                700000001,
                800000011,
                900000011,
                1000000007,
                1100000009,
                1200000041,
                1300000003,
                1400000023,
                1500000001,
                1600000009,
                1700000009,
                1800000011,
                1900000043,
                2000000011
            };
        }   // End of namespace detail
        
        size_t get_prime_number(size_t n) {
            size_t i;
            
            if ( n <= 0 )
                return detail::g_prime_numbers[0];
            for ( i = 0; i < ( sizeof(detail::g_prime_numbers)/ sizeof( detail::g_prime_numbers[0] ) ) ; i++) {
                if ( detail::g_prime_numbers[i] > n )
                    return detail::g_prime_numbers[i];
            }
            // TODO: Error log, need to expand the prime number table
            return 0;
        }
    }   // End of namespace common
}   // End of namespace argos