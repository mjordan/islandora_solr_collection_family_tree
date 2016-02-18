# The problem

We want to determine if an object, regardless of its content model or status as a child of a compound object, is in a specific collection or child collection.

# The solution

The most commonly implemented way to get an object's collection (or parent compound) memberships is to do a SPARQL query against the Resource Index. This can be slow. Since properties that describe an object's relationship with its parent(s) are indexed in Solr, it should be possible to query it instead of the RI to get an object's family tree.

# Pseudocode

Do a query Solr like this: `q=PID:islandora\:something&fl=RELS_EXT_isMemberOfCollection_uri_mt,RELS_EXT_isMemberOf_uri_mt,RELS_EXT_isConstituentOf_uri_mt`

* RELS_EXT_isMemberOfCollection_uri_mt, if present, will contain the object's collection memberships. Additional queries must be made to get the ancestor collections all the way up to the root collection.
* RELS_EXT_isMemberOf_uri_mt and RELS_EXT_isConstituentOf_uri_mt, if present, will contain the object's parent PIDs for paged and child compound objects respectively; if there are values for these, additional queries must be made to get the ancestor collections all the way up to the root collection.

# Proof of concept code

The drush command supplied with this module provides a proof of concept implementation this approach to getting all of the PIDs of collections and parents an object is part of. 

For example, 'booktest:4' is a page object in a book ('booktest:1'), which is in the 'islandora:bookCollection' collection. The root collection PID in the repopsitory is 'islandora:root'. Running the following command:

```
drush --user=admin igft --pid=booktest:4
```

produces the following output:

```
Ancestors of booktest:4 are booktest:1, islandora:bookCollection, islandora:root
```

This implementation detects multiple collection memberships, but its output simply lists the parents/collections, it doesn't express the relationships. For example, 'islandora:22' is an image that is in two collections, 'islandora:sp_basic_image_collection' and 'test:mycollection', which are both direct children of 'islandora:root':

```
drush --user=admin igft --pid=islandora:22
Ancestors of islandora:22 are islandora:sp_basic_image_collection, test:mytestcollection, islandora:top 
```

# Possible performance gains

* Do as few queries as possible, e.g., if we know that the object is not a page, don't look for RELS_EXT_isMemberOf_uri_mt
* Parallelize the Solr queries that get the membership relationship data
* Cache an object's family tree so we don't need to repeat the Solr queries
