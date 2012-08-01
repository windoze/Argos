FAQ
===

* Q. Why another search engine? We already have Lucene, Xapian, Sphinx, <you name it>...?
* A. The short answer is I don't like all of them.

    The long and rational answer is, none of them satasifies my requirements.
    
    + Lucene can do full-text search very well, but it lacks all other features, the worst part is, Lucene tries to put all kind of structural search features into full-text search category, which leads to inflexible design and complicated code that hard to extend. Check out Scorer if you haven't.
    + Xapian is also focusing on full-text search, it's design is better than Lucene for structural search, actually Xapian was the reverse-index implementation in the very early stage of Argos, but short awhile I've found some severe performance issues: Xapain took seconds to complete a very simple OR query with 3 keywords across 1.5 million documents. Then I abandoned Xapain and started a new reverse index implementation from the scratch.
    + Sphinx looks very similiar to Argos from the feature set point of view, and I've had a hard decision before I started Argos, but Sphinx lacks some very important features to me, such as text relevance scoring by user-supplied-expression and multi-value support(which was added later), also it behaves badly when handling a lot of concurrent connections and it eats a lot of memory if we enable real-time update.

* Q. Why should I use Argos if all data already organized into database? Can I just do search in database?
* A. Sure you can, but there are something need to be considered before doing so:
    + Most DBMS are designed to handle few long-live connections instead of a lot of short connections, this limits the QPS for most Web applications.
    + Most DBMS cannot do full-text search well, Argos works better.
    + Argos contains some features specially designed for search and histogram where equivalents cannot be found in most DBMS, such as RANGE and multi-value fields.
    + One important feature of DBMS is transaction. DBMS spends a lot to enforce data consistency between different transactions even you don't need them. Although this feature is pointless in most cases when we just need to search, most DBMS doesn't allow you to trade data consistency with performance and robust.

* Q. What? No Transaction?
* A. Search engine works on a snapshot of source data, the consistency is eventually maintained, but not strongly guaranteed like transaction.

    Yes, transaction is pointless for search, the data are out of date as soon as they've been delivered to users, there is no way to keep a transaction between your database server and user's browser.

* Q. Why C++? Why not Java?
* A. It's not because of language preference, it's about language capability.

    Argos does a lot dynamic evaluation during the search process, C++ allows me to use user-defined value type without involve dynamic memory allocation. A typical search request across 1m documents may generate 100m small objects represent the result of expression evaluation, it will kill Java with a lot of full-GC unless we use some tricky techniques such as object pool. A prototype of Argos was developed in Java and it suffered the issue, which made the prototype completely a toy.

    Not only Java, all JVM based languages are facing the same problem. .Net based languages could be better as they do have stack-based value type, but I've no time to dig deeper.

* Q. Why 64bit platform only?
* A. Argos uses mmap extensively, 32 bit platform doesn't give us big enough address space.

* Q. How about Windows?
* A. Argos currently supports 64bit POSIX-compliant platform only, it has been tested on Linux, Mac OS X, and FreeBSD. Windows support is not currently on my schedule, but it should not be too hard and can be done in a week.

* Q. Why index files are so big?
* A. My fault :) Well, index is so big with reasons, Argos supports real-time update with almost no delay, so the index files must not be compressed, and internal data structures contain a lot of reserved spaces for upcoming changes.

    Because of the nature of memory mapped file, Argos index files are sparse and not as big as 'ls' command tells, use 'du' command you can see real size should be much smaller (although still too big).
    
    There are a lot of improvements can be made to shrink index file size and some of them are already on the schedule.
    
* Q. Why indexer runs so slow?
* A. My fault, too :) Several reasons cause that. Argos uses mmap with very sparse data structures, which makes dirty pages scattered everywhere, indexing process eventually needs to flush all dirty pages onto disk, and traditional hard-disk performs badly with lot of random writing.

    A workaround is to build index on SSD or RAM disk (such as /dev/shm if it's big enough). You can have index of millions of documents build in few minutes.

    Just like index file size problem, there are also a lot of things can be improved.

* Q. I've Argos running, but 'top' command reports a ridiculously large number of virtual memory size that Argos uses, why?
* A. The number is true, Argos does use "ridiculously large" virtual address space, but just address space, not real memory, neither physical nor virtual.

    This is very unlikely to be changed and you may need to adapt you monitoring tool for this.
