MANUAL
======
Argos has 2 individual programs, indexer and searcher.

Indexer
-------

Indexer can build or update index from source file.

Indexer has following command line arguments:

* `-c|--config`, a config XML file, the format is described in the section 'Config.XML'. Indexer will try to create a new index if this argument exists, otherwise indexer will try to open an existing index at `--index-dir`.
* `-d|--index-dir`, the directory to store index, the directory must *not* exist if there is a config XML specified.
* `-i|--input-file`, input file name, indexer reads input data from stdin if the file name is `-` or this argument doesn't appear.

### Config.XML ###

The XML file defines index schema, which has root name `<config>`, and a child node `<fields>`.

In node `<fields>`, one or more fields can be defined by adding `<field>` node with following attributes:

* `name`, required, the name of the field, the name must starts with `[A-Za-z_]`, and followed by `[A-Za-z_0-9]*`, no other characters allowed. Names are case-sensitive.
* `type`, required, the type of the field, can be:
    + `integer`, a signed interger number.
    + `float`, a signed 4 bytes float number.
    + `double`, a signed 8 bytes double number.
    + `geolocation`, a pair of 4 bytes float numbers, first is latitude, then langitude.
    + `string`, a string with UTF-8 encoding. NOTE: Argos doesn't enforce character encoding, but indexing may fail if the string is not correctly encoded.
* `bytes`, optional, only usable when `type="integer"`, can be 1, 2, 4, or 8, default is 8.
* `multi`, optional, only usable when type is not `"string"`, can be `true` or `false`, the field is multi-value when `multi="true"`.
* `store`, optional, can be `"true"` or `"false"`, if `store="false"`, the field itself is not stored in the index and the field value cannot be retrieved, default is `"true"`.
* `index`, optional, can be `"true"` or `"false"`, the field will be indexed if `index="true"`. Different type of fields will be indexed differently.
* `namespace`, optional, only usable when `index="true"`, 2 or more fields can use same namespace, which means the HAS() will search terms in all of them. By default each field has different namespace; Fields with `namespace="-"` can be searched via term query.
* `analyzer`, optional, only usable when `index="true"` and `type="string"`, indicates how indexer break text into terms, can be:
    + `token`, seperate text by space, without more processes.
    + `text`, replace all punctuations with space; seperate text by space; convert all Latin letters to lower case; remove accent marks; and break CJK text into single ideograms.
    + `bigram`, same as above, with CJK text does not only generate single ideograms, but also BiGrams.

### Input File Format ###

The input file is basically CSV file, each line contains a single document.

NOTE: The first field of each document *must* be the primary key, which is implicit and *not* in config.xml file.

There are some exceptions from standard CSV format:

* Multi-value field has format `[v1,v2...]`, no quotes, empty field is `[]`.
* geolocation field has format `<lat,long>`, no quotes.

Indexer simply uses `std::getline()` to read from input file, so the maxium length of each document depends on the compiler and stdc++ lib. It should be ok to handle lines over 1M bytes on most systems.


Searcher
--------

Searcher has HTTP interface that supports GET method, POST is not supported.

Maximium request size is 64KB, this is hardcoded and can only be changed by re-compilation.

Searcher supports multiple indices at same time, each index has their own name, and operations to the index starts with prefix '/index_name'.

Searcher has following command-line arguments:

* `-d|--index-dir`, index directory, can be used multiple times to support multiple indices.
* `-a|--listen-addr`, listening address, default to `0.0.0.0`.
* `-p|--listen-port`, listening port, default to `8765`.
* `-r|--doc-root`, root directory for static contents, including XSLT files, default to `.`.

    You don't need it if you're not connecting to Argos with browsers.

* `-t|--threads`, number of threads to serve HTTP request, default to `3`.

    Argos threading model is effective enough so you should not use too many threads. A reasonable value is the number of CPU.

* `-l|--log-config`, log configuration file to be used, which is a [log4cplus](log4cplus.sf.net) log properties file.

    All logs are printed to stdout if the argument is missing.

### Query ###

URL prefix is `/index_name/query`, following parameters are supported:

* `m`, match condition
* `f`, filter expression
* `s`, sorting criteria
* `nr`, number of results to return
* `sk`, number of top results to skip, after sorting
* `fl`, returned fields
* `h`, histograms
* `fmt`, return format

#### Match condition ####

Match condition can be combination of following operators:

* `"text"`, return documents contains `"text"` in `namespace="-"`, this condition will be expanded into a serial of terms to match, using "text" analyzer.
* `AND(c1,c2,...cn)`, return documents match *all* conditions
* `OR(c1,c2,...cn)`, return documents match *any* conditions
* `HAS(field,"text)`, return documents have `field` match `"text"`, text is expanded with the analyzer specified for field, in config.xml
* `INFO([field,]* "text")`, this is a special query, it matches any documents, in addition, it retrieves match info for field and text, which can be used later in filter and sorting criteria.

Operators are case-sensitive.

#### Filter ####

Filter is a single expression, each document returned in match condition will be checked against with the expression, and all document with expression value 0 will be discarded.

Expression syntax is in prefix form, following functions are supported:

* Named constants, include `E`, `PI`, and `NOW`, `NOW` is a double number of seconds between current time and the epoch (1970-1-1).
* Math functions, include `ADD`, `SUB`, `MUL`, and `DIV`.
* Comparison, include `LT`, `LE`, `GT`, `GT`, `EQ`, `NE`, returns `0` or `1`.
* Logical functions, include `AND`, `OR`, `NOT`, `XOR`, these functions operate bit-wise for integral oprands, otherwise logical.
* Condition function, `IF(condition,true_clause,false_clause)` returns value of `true_clause` when `condition` evaluates to non-zero, otherwise `false_clause`.
* `LEN(v)`, returns number elements of array or length of string.
* `AT(array,idx)`, returns `array[idx]`
* `FIND(array,element)`, returns the index of element in array
* `DIST(geoloc1,geoloc2)`, returns spherical distance between geoloc1 and geoloc2 in meter, using radius of Earth.
* `MINDIST(geoloc1,[geoloc2,...])`, returns the nearest spherical distance between geoloc1 and all geolocations in the second parameter, which must be an array of gelocations.
* `RANGE(v,anchor1,anchor2,...anchorn)`, return `n` if `v` in `[anchorn, anchorn+1)`
* Document specific operators, start with '@'
    + `@DOCCOUNT`, returns number of documents in the index
    + `@DL(field)`, return number of terms in the field for current document, this value depends on the analyzer the field uses
    + `@HAS(field,"text")`, this operator is used with `INFO(field,"text")` in match condition, returns 0 or 1 if the field contains `"text"`. NOTE: Without INFO part in match condition, this operator may return 0 even if text is actually in the field

#### Sorting Criteria ####

Argos supports multiple sorting criteria, each criterion is `expression,[ASC|DESC]`, the expression is used to calculate the sorting key, supports same operators as in filter. ASC to sort in ascending order, DESC to descending.

Addition sorting criterion is used when former sorting keys are equal.


#### `nr` and `sk` ####

`nr` and `sk` are used to support pagination, i.e. `nr=15` and `sk=15` means to retrieve the 16th to 30th documents after sorting, first 15 documents are skipped.

#### Returned Fields ####

Returned fields are a serial of expressions, each expression can be a field name or any supported combination as in filter.

Trying to retrieve a field that is not stored will cause a "Bad request" response.

`fl` can also be an asterisk(`*`) solely, which retrieves all stored fields.

#### Histograms ####

Histogram list is a serial of histogram specifications seperated by semicolon(`;`), each histogram specification is one or more expression seperated by comma (`,`).

Use multiple histogram specifications to generated multiple isolated histograms. No grouping function such as "ROLLUP" or "CUBE" in Oracle.
Each histogram specification can be one expression, and the expression value is used as grouping key, if the expression returns array, each element in the array will be counted. In such case, the summary of counts in all groups will be larger than number of documents.

Histogram specification can also be multiple expressions, in such case, the grouping key is a composited key. If any expression returns array, each element in the array will be composited with other expression values. i.e. if one document has histogram spec evaluated to `[1,2,3],4,[5,6]`, the grouping keys will be `[1,4,5]`,`[1,4,6]`,`[2,4,5]`,`[2,4,6]`,`[3,4,5]`,`[3,4,6]`, the document will be counted into 6 different groups.

#### Return Format ####

`fmt` can be following value:

* `xml`, Argos outputs a XML file, NOTE: A XSLT link is also in the XML file, mordern browsers can render XML file with it into a human readable HTML format.
* `jsona`, outputs a JSON file, each document in the result is an array, no field names are returned.
* `jsonm`, outputs a JSON file, each document in the result is a map, with field names.
* `csv`, a CSV-like format used by indexer and searcher, not all info is returned, for test only.
* `pb`, Google ProtocolBuffers, the `.proto` file is at `Argos/serialization/result/proto`.

### Query by Primary Key ###

URL prefix is `/index_name/item`, with following parameters:

* `id`, value is a list of primary keys, seperated by comma `,`.
* `fl` and `fmt`, same as in Query section.

Number of returned documents always equals to the number specified in `id` parameter, and the order is same as the order specified in query. All values are empty if the document does not actually exist.

### Real-Time Update ###

Argos supports real-time update, URL prefix is `/index_name/insert` and `/index_name/update`

After the question mark(`?`), remain part is a document in the same format used by indexer.

For insertion, if the document has same primary key already exists in the index, old one is replaced by the new one, which increases erase count.

For updating, the document must still contain *all* fields, but all fields needn't to be updated should have value `null`.

Only non-indexed and stored fields can be updated, otherwise, you have to use `insert` to create/replace whole document.

Insertion and updating are asynchronous, there is no way to know if they are completed or failed, and no guarantee when the action will happen.

### Index Info ###

URL is `/indices.xml`, returns a XML file contains info for all indices.
