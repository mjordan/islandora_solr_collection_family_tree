<?php

/**
 * @file
 * Drush file for the Islandora Solr Collection Family Tree module.
 */

/**
 * Implements hook_drush_command().
 */
function islandora_solr_collection_family_tree_drush_command() {
  $items = array();
  $items['islandora_get_family_tree'] = array(
    'aliases' => array('igft'),
    'description' => "Get an object's family tree all the way up to the root collection.",
    'examples' => array(
      'drush igft --user=admin --pid=islandora:foo',
    ),
    'options' => array(
      'pid' => array(
        'description' => 'The PID of the object.',
      ),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
  );
  return $items;
}

/**
 * Get an object's family tree.
 */
function drush_islandora_solr_collection_family_tree_islandora_get_family_tree() {
  if (!module_exists('islandora_solr_metadata')) {
    drush_set_error('SOLR_SEARCH_NOT_AVAILABLE',
      dt('Sorry, Islandora Solr Search not enabled.'));
    drupal_exit();
  }

  // Confirm the specified object exists and is accessible; if it's not,
  // exit with an error.
  if (drush_get_option('pid')) {
    if (!islandora_object_load(drush_get_option('pid'))) {
      drush_set_error('OBJECT_NOT_FOUND',
        dt('The specified object (!pid) is not found or is not accessible.',
        array('!pid' => drush_get_option('pid'))));
      drupal_exit();
    }
  }

  $ancestors = islandora_solr_collection_family_tree_get_ancestors(drush_get_option('pid'));
  $ancestors = array_unique($ancestors);
  $tree = implode(', ', $ancestors);
  drush_print('Ancestors of ' . drush_get_option('pid') . ' are ' . $tree);
}

/**
 * Recursive function to walk up the collection/parent tree.
 *
 * @param string $pid
 *   The PID of the object to start at.
 *
 * @return array
 *   An array containing all of the collection or parent objects
 *   PIDs the start object is under.
 */
function islandora_solr_collection_family_tree_get_ancestors($pid) {
  $rels = array(
    'RELS_EXT_isConstituentOf_uri_mt',
    'RELS_EXT_isMemberOf_uri_mt',
    'RELS_EXT_isMemberOfCollection_uri_mt',
  );

  $repository_pid = variable_get('islandora_repository_pid', 'islandora:root');

  $ancestors = array();
  foreach ($rels as $rel) {
    $parent_memberships = islandora_solr_collection_family_tree_query_solr($pid);
    if ($parent_memberships->{$rel}) {
      foreach ($parent_memberships->{$rel} as $parent) {
        // Solr field values are stored as info URIs but we query Solr using
        // PIDs, so we need to remove the namespace..
        $parent = preg_replace('#info:fedora/#', '', $parent);
        $ancestors[] = $parent;
        // If we don't specify which relationship to return on, the return
        // can be triggered prematurely for deep-nested objects with
        // multiple parents.
        if ($rel == 'RELS_EXT_isMemberOfCollection_uri_mt' &&
          $parent == $repository_pid) {
          return $ancestors;
        }
        $grandparents = islandora_solr_collection_family_tree_get_ancestors($parent);
        if (!is_null($grandparents)) {
          foreach ($grandparents as $grandparent) {
            $grandparent = preg_replace('#info:fedora/#', '', $grandparent);
            $ancestors[] = $grandparent;
            if ($rel == 'RELS_EXT_isMemberOfCollection_uri_mt' &&
              $grandparent == $repository_pid) {
              return $ancestors;
            }
          }
        }
      }
    }
  }
}

/**
 * Queries Islandora's Solr index to get membership inforation.
 *
 * @param string $pid
 *   The PID to query Solr for.
 *
 * @return array
 *   An array of objects.
 */
function islandora_solr_collection_family_tree_query_solr($pid) {
  if (is_null($pid)) {
    return FALSE;
  }
  $pid = preg_replace('#:#', '\\:', $pid);
  $solr_url = variable_get('islandora_solr_url', 'http://localhost:8080/solr');
  if (!preg_match('#^http://#', $solr_url)) {
    $solr_url = 'http://' . $solr_url;
  }
  $query = $solr_url . '/select?version=1.2&rows=1&wt=json&json.nl=map&' .
    'fl=RELS_EXT_isMemberOfCollection_uri_mt,RELS_EXT_isMemberOf_uri_mt,RELS_EXT_isConstituentOf_uri_mt&' .
    'q=PID:' . $pid;
  $raw_result = drupal_http_request($query);
  if ($raw_result->code != 200) {
    return FALSE;
  }
  $query_result = json_decode($raw_result->data);
  if ($query_result->response->numFound < 1) {
    return FALSE;
  }

  $result_doc = $query_result->response->docs[0];
  return $result_doc;
}
