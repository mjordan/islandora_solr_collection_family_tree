# The problem

Sometimes we need to know if an Islandora object is "in" a particular collection, regardless of the object's content model or what its immediate parent collection is. A couple of specific use cases where this information wold be useful are:
* We want to determine whether an Islandora page object is a descendant of a specific collection because we want to do something with all objects that are in the collection, like apply a specific Drupal theme to them using [Islandora Themekey](https://github.com/mjordan/islandora_themekey).
* We want to represent an object's "family tree" to the user, showing all of its ancestor collections and parent objects.

# The solutions

The most commonly implemented way to get an object's collection (or parent compound object) memberships is to perform a SPARQL query against Fedora's Resource Index. This can be slow. Because properties that describe an object's relationship with its parent collections and objects are indexed in Solr (at least using discoverygarden's [basic solr configs](https://github.com/discoverygarden/basic-solr-config), it is possible to query Solr instead of the RI to get an object's family tree.

This approach is consistent with recent trends within the Islandora 7.x codebase to replace potentially expensive RI queries with Solr queries, e.g., [Islandora Solr Collection View](https://github.com/Islandora-Labs/islandora_solr_collection_view).

# Pseudocode

* Perform an initial query Solr for a PID that returns the fields containing collection or parent relationship data: `q=PID:islandora\:something&fl=RELS_EXT_isMemberOfCollection_uri_mt,RELS_EXT_isMemberOf_uri_mt,RELS_EXT_isConstituentOf_uri_mt`
  * If present in the result, RELS_EXT_isMemberOfCollection_uri_mt will contain the object's collection memberships.
    * Perform additional queries to get the ancestor collections all the way up to the root collection.
    * Add resulting collection PIDs to a list for use later.
  * If present in the result, RELS_EXT_isMemberOf_uri_mt and RELS_EXT_isConstituentOf_uri_mt will contain the object's parent PIDs for paged and child compound objects respectively.
    * Perform additional queries to get the ancestor parent objects and collections all the way up to the root collection.
    * Add resulting parent object and collection PIDs to a list for use later.
* Do what you want with the resulting list - check it to see if a particular collection that you want to apply policies to is in the list, render the list to the end user, etc.

# Proof of concept code

The drush command supplied with this module provides a proof of concept implementation for getting the PIDs of all collections and parent compound objects an object is part of. 

For example, 'booktest:10' is a page object in a book ('booktest:1'), which is in the 'islandora:bookCollection' collection. The root collection PID in the repopsitory is 'islandora:root'. Running the following command:

```
drush islandora_get_family_tree --pid=booktest10
```

produces the following output:

```
Ancestors of booktest:10 are booktest:1, islandora:bookCollection, islandora:root
```

This implementation detects multiple collection memberships, but its output simply lists the parents/collections, it doesn't express the structure of the relationships. For example, 'islandora:22' is an image that is in two collections, 'islandora:sp_basic_image_collection' and 'test:myimagecollection', which are both direct children of 'islandora:root':

```
drush islandora_get_family_tree --pid=islandora:22
Ancestors of islandora:22 are islandora:sp_basic_image_collection, test:myimagecollection, islandora:root
```

A more complex example is for object islandora:117, which is a child of the comound object islandora:133, which itself is at the bottom of this deeply nested hierarchicy of collections:

```
islandora:root
  islandora:compound_collection
    collection:level2
      collection:level3
        collection:level4
          collection:level5
            islandora:133
              islandora:117
```

islandora:117 is also an immediate child of the islandora:sp_large_image_collection, which is a child of islandora:root:

```
islandora:root
  islandora:sp_large_image_collection
    islandora:117
```

Issuing the following command:

```
drush islandora_get_family_tree --pid=islandora:117
```
produces the following output:

```
Ancestors of islandora:117 are islandora:133, collection:level5, collection:level4, collection:level3, collection:level2, islandora:compound_collection, islandora:root, islandora:sp_large_image_collection
```

# Possible performance optimizations

The proposed approach could potentially perform quite a few Solr queries, depending on how far the initial object is from the Islandora top-level collection, and the proof of concept implementation performs these queries within nested `foreach` loops. If the general approach is sound, implementations may want to find ways of reducing the number of queries or optimizing performance in other ways. For example:

* Do as few Solr queries as possible, e.g., if we know that the object is not a page, ignore RELS_EXT_isMemberOf_uri_mt
* Cache an object's family tree so we don't need to repeat the Solr queries
* Parallelize the Solr queries that get the membership relationship data

# Todo

* Test this approach against comparable RI queries, particularly on large repositories and with objects that will require a lot of Solr queries.
* Refine the overall algorithm to perform as few Solr queries as possible.

# Proposer

* [Mark Jordan](https://github.com/mjordan)
