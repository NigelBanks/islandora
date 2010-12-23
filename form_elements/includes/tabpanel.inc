<?php

// $Id$

/* File: tabpanel.inc
 * Author: 
 *  Nigel Banks
 * 
 * Description: 
 *  Allows forms to dynamically add new tabs to a tabpanel in a form.
 */

// this is a bit of a hack.  It iterates through the 
// form and assigns a dummy value to any field in the $post 
// data that is marked as #required.  This is so that the 
// AHAH method can still function with required fields that 
// are first passed to the core form validation function.
function _dummy_post_info_nigelb($form, $post, $key = '') 
{
  $children = element_children($form);
  if (count($children) > 0) {
     foreach ($children as $key) {
       $post = _dummy_post_info($form[$key], $post, $key);
    }
  }
  if ($key!='' && isset($form['#required']) && $form['#required'] == TRUE && trim($post[$key]) == '') {
   $post[$key] = 'not empty';
  }
  return $post;
}
//-------------------------------------------------------------------------------
// 
// Params:
// Returns:
//
//-------------------------------------------------------------------------------
function ife_find_tabpanel_element($form, &$keys = array())
{
  if (isset($form['#type']) && $form['#type'] == 'tabpanel') {
    return $form['tabpanel'];
  } 
  $children = element_children($form);
  foreach ($children as $key) {
    $found = ife_find_tabpanel_element($form[$key], $keys);
    if ($found !== FALSE) {
      $keys[] = $key;
      return $found;
    }
  }
  return FALSE;
}
//-------------------------------------------------------------------------------
// 
// Params:
// Returns:
//
//-------------------------------------------------------------------------------
function ife_tabpanel_ahah() 
{
  drupal_json(array('status' => TRUE, 'data' => "<div>Stuff</div>"));
  return;
  drupal_json(array('status'   => TRUE, 'data' => "<div>Stuff</div>"));
  if (!isset($_POST['form_build_id']))
    return;    
  /*
  $form_state = array('storage' => NULL, 'submitted' => FALSE);
  $form_build_id = $_POST['form_build_id'];
  $form = form_get_cache($form_build_id, $form_state);
  $args = $form['#parameters'];
  $form_id = array_shift($args);
  $form['#post'] = $_POST;
  $form['#redirect'] = FALSE;
  $form['#programmed'] = FALSE;
  $_POST = _dummy_post_info($form, $_POST);
  $form_state['post'] = $_POST;
  $form['#post'] = $_POST;  
  drupal_process_form($form_id, $form, $form_state);
  $form = drupal_rebuild_form($form_id, $form_state, $args, $form_build_id);
  $qt_form = ife_find_tabpanel_element($form); */

  $javascript = drupal_add_js(NULL, NULL, 'header');
  drupal_json(array(
      'status'   => TRUE,
      'data'     => "<div>Stuff</div>",
      'settings' => call_user_func_array('array_merge_recursive', $javascript['setting'])));
}
//-------------------------------------------------------------------------------
// 
// Params:
// Returns:
//
//-------------------------------------------------------------------------------
function ife_remove_tabpanel_submit($form, &$form_state) 
{
}
//-------------------------------------------------------------------------------
// 
// Params:
// Returns:
//
//-------------------------------------------------------------------------------
function ife_add_tabpanel_submit($form, &$form_state) 
{
  unset($form_state['submit_handlers']);
  form_execute_handlers('submit', $form, $form_state);
  $form_state['rebuild'] = TRUE;
}
//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
function theme_tabpanel_items($form)
{
  // var_dump($form);
  //exit();
}

//-------------------------------------------------------------------------------
// 
// Params:
// Returns:
//
//-------------------------------------------------------------------------------
function theme_tabpanel($element) 
{
  global $base_url;
  $path=drupal_get_path('module','islandora_form_elements');
  drupal_add_js($path.'/js/jquery-1.4.4.min.js');
  $scripts = drupal_add_js($path.'/js/jquery-ui-1.8.7.custom.min.js');
  unset($scripts['core']['misc/jquery.js']); // Ignore the system Jquery.
  drupal_add_css($path.'/css/ui-lightness/jquery-ui-1.8.7.custom.css');
  $title = $element['#title'];
  $name = str_replace(array('/', ':'), '-', $element['#name']);

  // Jquery
  $js = '$(document).ready(function() { 
        var tab_counter = 2; 
        var tabs = $("#' . $name . '").tabs({
          tabTemplate: "<li><a href=\'#{href}\'>#{label}</a><span class=\'ui-icon ui-icon-close\' style=\'float: left; margin: 0.4em 0.2em 0 0; cursor: pointer;\'>Remove Tab</span></li>",
          add: function( event, ui ) { $( ui.panel ).append( $("#' . $name . '-form-template").children().clone(true)); } 
        });
        function addTab() {
          tabs.tabs( "add", "#' . $name . '-" + tab_counter, "' . $title . '");
          tab_counter++;
        }
        $("#add-' . $name . '" ).live( "click", function() { 
          addTab();
        });
        $("#' . $name . ' span.ui-icon-close" ).live( "click", function() {
          var index = $( "li", tabs ).index( $( this ).parent() );
          tabs.tabs( "remove", index );
        });
        var test = $("#' . $name . '-1");
        $("#' . $name . '-1").append( $("#' . $name . '-form-template").children().clone(true))
  });';
  // Template for Tab Panel
  $output = '<script type="text/javascript">'.$js.'</script>';
  $output .= '<div id='  . $name . '>';
  $output .= '<ul>';
  $output .= '<li><a href="#' . $name . '-1">' . $title . '</a><span id="add-' . $name . '" class="ui-icon ui-icon-circle-plus" style="float: left; margin: 0.4em 0.2em 0 0; cursor: pointer;"></span></li>';
  $output .= '</ul>';
  $output .= '<div id="' . $name . '-form-template" style="display:none">';
  $output .= '<fieldset'. drupal_attributes($element['#attributes']) .'>'. ($element['#title'] ? '<legend>'. $element['#title'] .'</legend>' : '') . (isset($element['#description']) && $element['#description'] ? '<div class="description">'. $element['#description'] .'</div>' : '') . (!empty($element['#children']) ? $element['#children'] : '') . (isset($element['#value']) ? $element['#value'] : '') . '</fieldset>';
  $output .= '</div>';
  $output .= '<div id="' . $name . '-1"></div>';
  $output .= '</div>';
  return $output;
}
