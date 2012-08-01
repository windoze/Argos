README
======

Argos is an Open Source Vertical Search Engine, it searches structural data with rich features.

Argos is a complete application suite can be used to serve vertical business out of box, and it's very extensible which allows user to add even more features.

Argos is not (just) a full-text search engine
---------------------------------------------
Argos does have a lot of full-text search features, including keyword matching, text relevance scoring, etc. But full text search is not the best scenario Argos may work. The most powerful of Argos is the part after match. Especially, Argos can use any expression to filter, sort, and compile statistics, and keep data up-to-date with almost no delay.

In online industries just like e-Commerce or local search, it's very typical that you already have data organized in databases with schema, but the problem is how to retrieved data meet complex criteria while the result set needs to be sorted by equations composed by many dynamic factors.

Real-time updating is also a big problem to most search engines, existing solutions such as Lucene/Solr can only do "Near Real Time Update" and the performance degrades significantly after many updates.

In Argos, elaborate design makes Argos can update thousands of documents in a second without slowing down queries.

Argos is built to solve these kind of problems.

How does Argos work and what Argos can do
-----------------------------------------
In Argos, a search request is broken into several phases.

1. The first one is full-text search. Argos generates reverse index for each field of documents, so it can match documents with keywords, and match criteria can be any AND/OR composition of keyword search. This phase basically achieves same feature as other full-text search engines.
2. The second phase is to filter matched document set with any expression as the condition. This phase can do something like "discard any shop out of 1 mile range from my location".
3. The third phase is to sort filtered document set with multiple expressions as the sorting keys. This phase can do something like "Nearest shop on top", also this phase limits the result set into specific size with number of results to skip, for pagination.
4. The last phase is actually executed at the same time as 3rd, which compiles statistics of results and generates histogram.

Let's take a look at a real-world example, assume we have an index for local shops, with following schema:

1. shopname, a string contains name of shop, which is indexed.
2. POI, a geolocation contains the latitude and longitude of the shop
3. userfeedback, an integer from 1 to 5, higher is better
4. categoryid, one or more category ids, indicates the shop type

Here is a query:

`/localshops/query?m="coffee shop"&f=LT(DIST(poi,[31.229939,121.54964]),1600)&s=userfeedback,DESC,DIST(poi,[31.229939,121.54964]),ASC&h=categoryid&fl=_id,shopname,DIST(poi,[31.229939,121.54964])&nr=10&sk=30&fmt=xml`

Above query searches a index called "localshops" for all shops contain both "coffee" and "shop" in shopname within 1 mile range to a given location; sort the result set by userfeedback score in descanding order, sort shops have same userfeedback score by the distance in ascending order; also the query generates histogram of shops by category, return a map from categoryid to count of shops belong to the category.

Result is in XML format; returned fields include shop id, name, and the distance. If there are 10 shops per page, the 4th page is returned.
