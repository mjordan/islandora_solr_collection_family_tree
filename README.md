# The problem

We want to determine whether an object, regardless of its content model or status as a child of a compound object, is in a specific collection or child collection.

# The solution

The most commonly implemented way to get an object's collection (or parent compound) memberships is to do a SPARQL query against the Resource Index. This can be slow. Since properties that describe an object's relationship with its parent collections and objects are indexed in Solr (at least using [DGI's basic solr configs](https://github.com/discoverygarden/basic-solr-config), it should be possible to query it instead of the RI to get an object's family tree.

This approach is consistent with recent trends within the Islandora 7.x codebase to replace potentially expensive RI queries with Solr queries, e.g., [Islandora Solr Collection View](https://github.com/Islandora-Labs/islandora_solr_collection_view).

# Pseudocode

Perform an initial query Solr for a PID that returns the fields that contain collection or parent relationship data: `q=PID:islandora\:something&fl=RELS_EXT_isMemberOfCollection_uri_mt,RELS_EXT_isMemberOf_uri_mt,RELS_EXT_isConstituentOf_uri_mt`

* If present in the result, RELS_EXT_isMemberOfCollection_uri_mt will contain the object's collection memberships.
  * Perform additional queries to get the ancestor collections all the way up to the root collection.
  * Add resulting collection PIDs to a list for use later.
* If present in the result, RELS_EXT_isMemberOf_uri_mt and RELS_EXT_isConstituentOf_uri_mt will contain the object's parent PIDs for paged and child compound objects respectively.
  * Perform additional queries to get the ancestor parent objects and collections all the way up to the root collection.
  * Add resulting parent object and collection PIDs to a list for use later.
* Do what you want with the resulting list - check it to see if a particular collection is in it, etc.

# Proof of concept code

The drush command supplied with this module provides a proof of concept implementation this approach to getting all of the PIDs of collections and parents an object is part of. 

For example, 'booktest:10' is a page object in a book ('booktest:1'), which is in the 'islandora:bookCollection' collection. The root collection PID in the repopsitory is 'islandora:root'. Running the following command:

```
drush --user=admin igft --pid=booktest10:
```

produces the following output:

```
Ancestors of booktest:10 are booktest:1, islandora:bookCollection, islandora:root
```

This implementation detects multiple collection memberships, but its output simply lists the parents/collections, it doesn't express the relationships. For example, 'islandora:22' is an image that is in two collections, 'islandora:sp_basic_image_collection' and 'test:myimagecollection', which are both direct children of 'islandora:root':

```
drush --user=admin igft --pid=islandora:22
Ancestors of islandora:22 are islandora:sp_basic_image_collection, test:myimagecollection, islandora:root
```

# Possible performance optimizations

The proposed approach performs could perform a lot of Solr queries depending on how far the initial object is from the Islandora top-level collection, and the proof of concept performs these queries within nested `foreach` loops. If the general approach is sound, implementations may want to find ways of reducing the number of querieres or optimizing performance in other ways. For example:

* Do as few Solr queries as possible, e.g., if we know that the object is not a page, ignore RELS_EXT_isMemberOf_uri_mt
* Cache an object's family tree so we don't need to repeat the Solr queries
* Parallelize the Solr queries that get the membership relationship data

# Todo

* Test this approach against comparable RI queries, particularly on large repositories and with objects that will require a lot of Solr queries.
* Refine the overall algorithm to perform as few Solr queries as possible.

# Proposer

* [Mark Jordan](https://github.com/mjordan)
