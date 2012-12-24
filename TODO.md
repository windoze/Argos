TODO
====

BUGS
----
* Escape '"' in CSV serializer for output
* Argos currently is *not* C++11 clean

Features still missing
-----------------------
* Refine serialization
    + Memory usage of serialization is way too suboptimal, JSON serialization uses json_spirit which needs more attention.
* Histogram
    + Now histogram can only count docs, summary/average/... are still missing
* Grouping
    + Most of things for grouping are done in histogram implementation, only thing left is to record doc ids instead of counting them, a `block_chain<primary_key>` should be enough, another thought is to add sort/nr to each group to limit items output, in such case we need to use random-accessible containers instead of block_chain as it's a linear container just like linked list.
* Aggregator

    Aggregator can aggregate results from different searchers, as long as each searcher contains different part of doc set. Somethings still need to be considered, the biggest problem is some whole-doc-set values such as DOCCOUNT and IDF, these values cannot be calculated on individual searcher, they need to be collected from all searchers and summarized in aggregator, then we need to transfer these values from to searchers before doing actual search, this may require 2 or more roundtrips between A/S, and may impact performance.

    Despite this issue, other features are pretty easy to be implemented.

Short/Mid term, need improvement
--------------------------------
* Optimize memory usage.

    Now HTTP part is half-optimized as reply::content is still out there, this one is hard to move into mem_pool due to reasons:

    + STL containers in C++ prior C++0x/11, such as vector, basically cannot pass a non-default-constructable allocator to elements inside element, which makes `vector<basic_string<char, alloc>, alloc>` unusable, we had to use a default-constructable, thread-local/global allocator.
    + Boost.asio will strand states within different threads, which makes us cannot thread-local allocator to save any data need to be transferred in and out.

    C++11 can be a cure as long as we implement `allocator_traits<mem_pool_allocator>` to support scoped_allocator_adaptor, then we can move mem_pool from thread to connection.

* Java-JDBC/Python-DBAPI/Perl-DBD/Ruby-ActiveRecord adaptor, will be more useful if we also support SQL-like syntax. By providing a database like interface, the program can be seamlessly integrated with a lot of existing libraries/frameworks.

* Join across multiple indices

    Now Argos can serve multiple indices in one process, need evaluation about how to join across them.

Long term, low priority
-----------------------

* Refine value list parser

    Currently value list parser only support a CSV-like format, which limits indexing function.

    + Add JSON array and JSON map parser
    + Add XML parser

* UDP based multi-cast realtime updating

    This is very useful for a cluster of replicating servers contain same data set and need to be updated at same time. The hard part is, UDP messages may be lost during the transferring, so we need a loss-detection/loss-recovery mechanism to maintain consistency.

* Indexing time is too long

    The program uses mmap everywhere, and calls msync when it needs to save data to disk. After a lot of updates, there will be a lot of dirty pages and need long time to be written. Traditional HD performances badly on random writing, current workaround is to build index on SSD or even RAM disk.

    Also we can break index into small parts and merge them, but this feature is still missing.

* Index is too big

    All data on disk is *not* compressed, and there are a lot of gaps between data structures, this is necessary to make the index to be read and updated efficiently. But there are still a lot of things can be done to make index files smaller.

* BM11/BM11F/BM25/BM25F

    We already have enough info, these values can be easily calculated, but again, aggregator can be a problem.

    Besides, many customized weighting schema should be doable just in query, we can already extract a lot of info during the sorting phase.

* Phrase/near match

    Need major change to reverse index, and almost every part in query part.

* Wildcard

    This can be done by setting up a dictionary for terms, then use this dictionary to get an expanded set of term for given term prefix, then an OR query will do rest of things. The problem is the expanded term set may be very large and may cause search performance degrade vastly, need a full measurement before starting.

* Search suggestion

    This can be done by (1)wildcard, or (2)another index for all popular queries. Both approaches need to be figured thoroughly before starting.

* Smarter analyzer, especially for CJK texts.

    Now we have a token analyzer which breaks space-seperated text; a text analyzer which breaks CJK text into single ideograms; also a BiGram analyzer which breaks CJK text into single ideograms and bi-grams.

    We still need a more intelligent analyzer such as HMM to break CJK text more properly.

* Stemmer and spell checker for Latin texts

    Now we filtered all accent mark and decomposit composited Latin letters, but we still need a stemmer and spell checker.

* Highlighter

    Highlight match text in returned texts, this can be done with `match_info`.

* More serializers

    Now we have ProtocolBuffers, can support other formats such as Avaro, MsgPack, and BSON, still have no time to look into details, but I think these should not be too hard to implement.

* Network protocol other than HTTP

    SPDY is under consideration, need time to look into protocol details.

* In-fix query syntax similiar to SQL. Which should be useful as prefix syntax is easy to parse but hard to read and write. A SQL-like query language can ease the learning curve.
