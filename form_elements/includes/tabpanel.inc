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
    return $form;
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
function remove_tabpanel(&$form, $clicked_button) 
{
  $parents = $clicked_button['#array_parents'];
  $index =  $clicked_button['#value'];
  $depth = count($parents) - 2;
  $point = &$form;
  for($i = 0; $i < $depth; $i++) { 
    $point = &$point[$parents[$i]];
  }
  $count = 1;
  while($point[$count])
    $count++;
  for($i = $index; $i < $count; $i++) {
    $point[$i] = $point[$i+1];
  }
  unset($point[$count-1]);
}
function find_tabpanel_to_insert(&$form, $clicked_button)
{
  $offset =  ($clicked_button['#action'] = 'remove')? 3 : 1;
  $parents = $clicked_button['#array_parents'];
  $index =  $clicked_button['#value'];
  $depth = count($parents) - $offset;
  $point = &$form;
  for($i = 0; $i < $depth; $i++) { 
    $point = &$point[$parents[$i]];
  }
  return $point;
}
//-------------------------------------------------------------------------------
// 
// Params:
// Returns:
//
//-------------------------------------------------------------------------------
function ife_tabpanel_ahah() 
{
  if (!isset($_POST['form_build_id']))  
    return;    
  $form_state = array('storage' => NULL, 'submitted' => FALSE, 'post' => $_POST);
  $form_build_id = $_POST['form_build_id'];
  if (!$form = form_get_cache($form_build_id, $form_state))
    exit();
  $form_id = $form['#parameters'][0];
  $form['#post'] = $_POST;
  $form['#programmed']  = FALSE;
  //  $form = form_builder($form_id, $form, $form_state);
  $output = $form;
  drupal_json(array(
      'status'   => TRUE,
      //'data'     => theme('status_messages') . drupal_render($output),
      'data'     => theme('status_messages') . '<div>'.print_r($output,TRUE).'</div>',
  ));
  return; 

  $form_state = array('storage' => NULL, 'submitted' => FALSE, 'post' => $_POST);
  $form_build_id = $_POST['form_build_id'];
  if (!$form = form_get_cache($form_build_id, $form_state))
    exit();
  $form_id = $form['#parameters'][0];
  $form['#post'] = $_POST;
  $test = $form['indicator2'][0]['mods-titleInfo'];
  drupal_process_form($form_id, $form, $form_state);
  $clicked_button = $form_state['clicked_button'];
  $form['indicator2'][0]['mods-titleInfo'] = $test;
  switch($clicked_button['#action']) {
    case 'remove':
      remove_tabpanel($form, $clicked_button);
      break;
    case 'add':
      add_tabpanel($form, $clicked_button);
      break;
    default:
      exit();
  }
  $form = form_builder($form_id, $form, $form_state);
  $output = find_tabpanel_to_insert($form, $clicked_button);
  unset($output['#prefix'],$output['#suffix']);
  $javascript = drupal_add_js(NULL, NULL, 'header');
  drupal_json(array(
      'status'   => TRUE,
      'data'     => theme('status_messages') . drupal_render($output),
      //'data'     => theme('status_messages') . '<div>'.print_r($output,TRUE).'</div>',
      'settings' => call_user_func_array('array_merge_recursive', $javascript['setting']),
  ));
}
//-------------------------------------------------------------------------------
// 
// Params:
// Returns:
//
//-------------------------------------------------------------------------------
function ife_remove_tabpanel_submit($form, &$form_state) 
{
  $clicked_button = $form_state['clicked_button'];
  switch($clicked_button['#action']) {
    case 'remove':
      remove_tabpanel($form, $clicked_button);
      break;
    case 'add':
      add_tabpanel($form, $clicked_button);
      break;
  }
  $form_id = $form['#parameters'][0];
  $form['#post'] = $_POST;
  $form['#programmed']  = FALSE;
  $form = form_builder($form_id, $form, $form_state);
  unset($form_state['submit_handlers']);
  form_execute_handlers('submit', $form, $form_state);
  $form_state['rebuild'] = FALSE;
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
  $form_state['rebuild'] = FALSE;
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
  $path = drupal_get_path('module','islandora_form_elements');
  $scripts = drupal_add_js($path.'/js/jquery-ui-1.8.7.custom.min.js');
  unset($scripts['core']['misc/jquery.js']); // Ignore the system Jquery.
  drupal_add_css($path.'/css/ui-lightness/jquery-ui-1.8.7.custom.css');
  $name = $element['#name'];
  $count = 1;
  $removeFunctions = '';
  while($element['content'][$count]) {
    $id =  $element['content'][$count]['remove_tab']['#id'];
    $removeFunctions .= '$("#remove-' . $name . '-'. $count .'").live("click", function() { $("#'. $id .'").trigger("mousedown"); });';
    $count++;
  }
  $js = '$(document).ready(function() { 
        var tabs = $("#' . $name . '").tabs();
        $("#add-' . $name . '").live("click", function() { $("#' . $element['add-tab']['#id'] . '").trigger("mousedown"); });
        $("#'.$name.' .ui-icon-close").live("click", function() { 
           var index = $(this).text();
           var id = "#'.$name.'-" + index;
           $(id).children(":last").trigger("mousedown"); });
        $("body").ajaxComplete(function() { var tabs = $("#' . $name . '").tabs(); });
  });';
  drupal_add_js($js, inline);
  return '<div id=' . $name . '>'.$element['#children'].'</div>';
}
//-------------------------------------------------------------------------------
function theme_tabpanel_items($form) 
{
  $name = $form['#name'];
  $title = $form['#title'];
  $output = '<ul>';
  $output .= '<li><a href="#' . $name . '-0">' . $title . '</a><span id="add-' . $name . '" class="ui-icon ui-icon-circle-plus" style="float: left; margin: 0.4em 0.2em 0 0; cursor: pointer;"></span></li>';
  $count = 1;
  while($form[$count]) {
    $output .= '<li><a href="#' . $name . '-'.$count.'">' . $count . '</a><span id="remove-' . $name . '-'. $count .'" class="ui-icon ui-icon-close" style="float: left; margin: 0.4em 0.2em 0 0; cursor: pointer;">'.$count.'</span></li>';
    $count++;
  }
  $output .= '</ul>';
  $count = 0;
  while($form[$count]) {
    $output .= '<div id="' . $name . '-'.$count.'">';
    $output .= drupal_render($form[$count]);
    $output .= '</div>';
    $count++;
  }
  return $output;
}