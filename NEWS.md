## 0.3.00.00 [2019-04-11]
- In `carbon-tool`, enable the user to set whether a single-threaded (`sync`) or multi-threaded (`async`) string 
  dictionary should be used when conversion from JSON to CARBON archives is issued (via `convert` module). By
  default a multi-threaded implementation with 8 threads is used. To change that, `carbon-tool convert` has
  now to new parameters, `--dic-type` and `--dic-nthreads`. See `carbon-tool` usage instructions for details.
- Modified command line in module `convert` of `carbon-tool` to select compressor to be used for conversion 
  from JSON to CARBON archives. Until now, `--compressor=<compressor>` was used. Now, it is just 
  `--compressor <compressor>`.
- In Compressor Framework, add function to read implementation-specific extra data (e.g., in order to reconstruct
  a coding table). See `read_extra` of `carbon_compressor_t` in [carbon-compressor.h](include/carbon/compressor/compressor.h)  
- Fix a bug that permits to import empty JSON-objects (e.g., `{ "k": {} }`) in archives 
- Refactoring and cleanup

## 0.2.00.01 [2019-03-25]
- To avoid re-creation of the index used for *Indexed String Fetch* when a archive is loaded into memory, 
  this index can now be pre-computed an embedded into the archive file itself (which is now the default behavior). 
  For this, the index is built after archive creation and appended at the end of the archive (if not turned-off). 
  To turn it off in `carbon-tool`, use flag `--no-string-id-index` for the module `convert`. If turned-off, the 
  index may be created during runtime depending on whether such an index should be used or not (see below).
  - Archive specification must be changed for this, see updated [SPECIFICATION.md](SPECIFICATION.md) and 
    [SPECIFICATION-COMPACT.md](SPECIFICATION_COMPACT.md).

## 0.2.00.00 [2019-02-27]
- Add archive visitor framework, see [archive visitor framework](include/carbon/archive/archive_visitor.h).
- Add in-memory key-value pair representation of unstructured JSON-like objects 
  (which still use [object identifiers](include/carbon/oid/oid.h) and encoded strings) in preparation to 
  print CARBON files in typical key-value pair structure, see [encoded documents](include/carbon/json/encoded_doc.h).
    - Used [archive visitor framework](include/carbon/archive/archive_visitor.h) to convert 
      [CARBON archives](include/carbon/archive/archive.h) to an
      [encoded document collection](include/carbon/json/encoded_doc.h)
- *I/O Improved String Fetch* To avoid I/O with the file system as much as possible - given a particular
  amount of memory reserved to hold (uncompressed) string in main memory -, a string fetch by default
  first accesses a LRU hash-table-based cache (*Cached String Fetch*). In case of a cache miss, the string id is translated in-memory to
  its corresponding (potentially compressed) string offset in the archive file, and fetched directly avoiding
  a full scan (*Indexed String Fetch*). In detail:
    - *Indexed String Fetch*: Add archive-local index that maps from string ids to the offsets in the archive file where 
       the strings associated with those ids are located. Using this index avoids a *full scan* to find the string offset
       in the file. However, such an index is automatically created (once) `carbon_archive_query` is called. To
       avoid creation of this index, the index creation can be bypassed by calling `carbon_query_create` on the
       desired archive. Additionally, an index can be dropped during the lifetime of an archive by 
       calling `carbon_archive_drop_indexes`. Once an archive is closed, indexes gets dropped automatically (if any).
    - *Cached String Fetch*: Add archive-local cache that holds strings associated with string ids uncompressed
       in memory until those strings get evicted from the cache due to space limitations. As for indexed string
       fetch, such a cache is automatically generated (once) when `carbon_archive_query` is called but also
       can be bypassed in the same way as the index creation for indexed string fetches. Depending on the
       cache size and data access pattern, accesses to the underlying data provider (i.e., the index for string 
       fetches or a string fetch via a full scan) can be significantly avoided if not completely avoided.  
       Internally, the cache is organized as a (robin hood) hash table using the bernstein hash function that
       uses the string id as key to find a fitting bucket. Inside the bucket, an LRU list for entries
       with string ids that have a collision under the hash function is maintained.
- Add `$ carbon-tool to_json <input>` to convert a CARBON archive file `<input>` into its JSON representation.  

## 0.1.00.06 [2019-02-23]
- Completed compressor framework including embedding into carbin archive operations 
  (e.g., automatically call <code>encode</code> or <code>decode</code> when needed)
    - Compressor _none_ works fully, _huffman_ still cannot <code>decode</code>
- Implement query function to archives
    - *String Identifier Scan*: Iteration through all string ids (including offset and string length) 
      stored in an archives string table, see <code>carbon_query_scan_strids</code>. Iterator model 
      is vector-based to minimize I/O calls to the underyling disk file, 
      see [carbon-strid-iter_t](include/carbon/archive/strid-iter.h)
    - *String Fetch*: Fetch a stored (and potentially compressed) string in an archives string 
      table given its string identifier. For generic and safe calls, this fetch performs *full scan*
      iteration using [carbon-strid-iter_t](include/carbon/archive/strid-iter.h) seeking for the desired
      string id (if a string id is found), see <code>carbon_query_fetch_string_by_id</code>. 
      For fast and unsafe calls having the offset to the desired string id in the archives string table 
      at hand, a _random access_ into the archives underlying disk file is performed, see
      <code>carbon_query_fetch_strings_by_offset</code>. To avoid too many open files and to guarantee 
      thread-safe behavior, such an unsafe operation is invoked via a 
      [carbon-io-context_t](include/carbon/archive/io_context.h). In both case, the string
      gets decompressed using the compressor framework when it gets fetch into main memory.
  - *Id Fetch*: Fetch string ids where the referenced string matches a particular condition,
      see <code>carbon_query_find_ids</code>. Such a condition, implemented as (vectorized) predicate 
      function, can be freely defined by extending the [String Predicate Framework](include/carbon/archive/string-pred.h) with
      user-defined predicates. There are already some [built-in predicates](include/carbon/string-pred),
      such as [equals](include/carbon/string-pred/carbon-string-pred-equals.h) or 
      [contains](include/carbon/string-pred/carbon-string-pred-contains.h). Internally, a string
      identifier scan is invoked. For the entire vector for such an iteration step, a string
      fetch with random access is invoked, yielding the corresponding uncompressed strings to the
      string identifiers inside the vector. For all of these strings, the predicate is invoked
      resulting in the indices inside the vector where the strings match the condition. Marked
      indices are added to the result set. Depending on the predicate and the users choice, a limit
      to the result set size can be set, stopping the fetch potentially earlier.
- Implemented iterator model to iterate trough CARBON archives in order to fully exploit zero-cost access
  to columnar stored key and value data, see [iterators](include/carbon/archive/archive_iter.h). 

## 0.1.00.05 [2019-02-13]
- **Compressor Framework** refactored compressor framework. 
    - Extension with new compressors in [carbon-compressor.c](src/carbon/compressor/compressor.c)
    - Current compressors are located in [include/carbon/compressor](include/carbon/compressor)
    - Available compressors now listed via `carbon-tool list compressors`,
      and selection is done via `carbon-tool convert --use-compressor=$NAME` where 
      `$NAME` is from the list above 

## 0.1.00.04 [2019-02-13]
- Unique object identifiers added in CARBON files, see [oid.c](src/carbon/oid/oid.c)
- Fine-grained switched to turn on and turn of logging, see [INSTALL.md](INSTALL.md)
- Default build type is now release
- Contains the following bug fixes 
    - [size-optimized (Huffman) fails in edge cases](https://github.com/protolabs/libcarbon/issues/1)

## 0.1.00.00 [2019-02-07]

- Initial standalone release
- Set license to MIT
- Refactoring and cleanups
