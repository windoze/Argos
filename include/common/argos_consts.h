//
//  argos_consts.h
//  Argos
//
//  Created by Windoze on 12-7-5.
//  Copyright (c) 2012 0d0a.com. All rights reserved.
//

#include <stddef.h>
#include <stdint.h>

#ifndef Argos_argos_consts_h
#define Argos_argos_consts_h

namespace argos {
    /**
     * type docid is 4-bytes unsigned integer, the index can hold 4G-2 documents at max, includes deleted
     */
    typedef uint32_t docid;
    
    /**
     * docid 0 is not used
     */
    const docid UNUSED_DOC=0;
    
    /**
     * the maxium value of docid indicates no doc
     */
    const docid INVALID_DOC=0xFFFFFFFF;
    const docid MAX_DOC=0xFFFFFFFF;
    
    /**
     * type primary_key is 8-bytes unsigned integer
     *
     * TODO: support other primary key types, i.e. 128bit UUID or 160bit SHA1 hash value
     * TODO: support index without primary key?
     */
    typedef uint64_t primary_key;
    
    /**
     * Invalid primary key value
     */
    const primary_key INVALID_PK=primary_key(-1);
    
    /**
     * test if a docid is valid
     *
     * @param docid the docid to check
     * @return true if the docid is valid
     */
    inline bool is_valid(docid did) {
        return did!=UNUSED_DOC && did!=INVALID_DOC;
    }
}   // End of namespace argos

#endif
