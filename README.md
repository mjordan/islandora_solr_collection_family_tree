# Basic idea

The best way to get an object's collection membership is to do a SPARQL query against the Resource Index. This can be very slow. Since properties that describe an object's relationship with its parent(s) is indexed in Solr, it should be possible to query it instead of the RI to get an object's family tree.

# Pseudocode

Do a query Solr like this: `q=PID:islandora\:something&fl=RELS_EXT_isMemberOfCollection_uri_mt,RELS_EXT_isMemberOf_uri_mt,RELS_EXT_isConstituentOf_uri_mt`

* RELS_EXT_isMemberOfCollection_uri_mt, if present, will contain the object's collection memberships. Additional queries must be made to get the ancestor collections all the way up to the root collection.
* RELS_EXT_isMemberOf_uri_mt and RELS_EXT_isConstituentOf_uri_mt, if present, will contain the object's parent PIDs; if there are values for these, additional queries must be made to get the ancestor collections all the way up to the root collection.

# Possible performance gains

* Can we parallelize these queries?
