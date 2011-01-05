
<?php

// $Id$

/* File: tabpanel.inc
 * Author: 
 *  Nigel Banks
 * 
 * Description: 
 *  Allows forms to dynamically add new tabs to a tabpanel in a form.
 */

//-------------------------------------------------------------------------------
// _ife_remove_tabpanel_from_form
// Params:
//  form - 
//  clicked_button -
// Returns:
//
//
//-------------------------------------------------------------------------------
function _ife_remove_tabpanel_from_form(&$form, $clicked_button) 
{
  $parents = $clicked_button['#array_parents'];
  $index =  $clicked_button['#tab'];
  $depth = count($parents) - 2;
  $point = &$form;
  for($i = 0; $i < $depth; $i++) { 
    $point = &$point[$parents[$i]];
  }
  $content = &$point;
  $count = 1;
  while($content[$count])
    $count++;
  for($i = $index; $i < $count; $i++) {
    $content[$i] = $content[$i+1];
  }
  unset($content[$count-1]);
}
//-------------------------------------------------------------------------------
// _ife_remove_tabpanel_values
// Params:
//  form -
//  clicked_button -
// Returns:
//
//
//-------------------------------------------------------------------------------
function _ife_remove_tabpanel_values(&$form, $clicked_button) 
{
  $parents = $clicked_button['#parents'];
  $index =  $clicked_button['#tab'];
  $depth = count($parents) - 2;
  $point = &$_POST;
  for($i = 0; $i < $depth; $i++) { 
    $point = &$point[$parents[$i]];
  }
  $content = &$point;
  $count = count($content);
  for($i = $index; $i < $count; $i++) {
    $content[$i] = $content[$i+1];
  }
  unset($content[$count-1]);
}
//-------------------------------------------------------------------------------
// ife_remove_tabpanel
// Params:
//  form - 
//  clicked_button -
// Returns:
//
//
//-------------------------------------------------------------------------------
function ife_remove_tabpanel(&$form, $clicked_button) 
{
  _ife_remove_tabpanel_from_form(&$form, $clicked_button);
  _ife_remove_tabpanel_values(&$form, $clicked_button);
}
//-------------------------------------------------------------------------------
// ife_add_tabpanel
// Params:
//  form -
//  clicked_button -
// Returns:
//
// 
//-------------------------------------------------------------------------------
function ife_add_tabpanel(&$form, $clicked_button) 
{
  $parents = $clicked_button['#array_parents'];
  $depth = count($parents) - 1;
  $point = &$form;
  for($i = 0; $i < $depth; $i++) { 
    $point = &$point[$parents[$i]];
  }
  $content = &$point['content'];
  $count = 1;
  while($content[$count])
    $count++;
  $content[$count] = $content[0];
}
//-------------------------------------------------------------------------------
// ife_find_tabpanel_to_insert
// Params:
//  form - 
//  clicked_button -
// Returns:
//
//
//-------------------------------------------------------------------------------
function ife_find_tabpanel_to_insert(&$form, $clicked_button)
{
  $offset = ($clicked_button['#action'] == 'remove')? 3 : 1;
  $parents = $clicked_button['#array_parents'];
  $depth = count($parents) - $offset;
  $point = &$form;
  for($i = 0; $i < $depth; $i++) { 
    $point = &$point[$parents[$i]];
  }
  return $point;
}
//-------------------------------------------------------------------------------
// is_clicked_button
// Params:
//  form_element - a form element.
// Returns:
//
//
//-------------------------------------------------------------------------------
function is_clicked_button(&$form_element)
{
  if (isset($form_element['#input']) && $form_element['#input']) {
    if (isset($form_element['#executes_submit_callback'])) {
      $name = $form_element['#name'];
      $value = $form_element['#value']; 
      if (isset($_POST[$name]) && $_POST[$name] == $value) {
	return TRUE;
      }
    }
  }
  return FALSE;
}
//-------------------------------------------------------------------------------
// ife_clicked_button
// Params:
//  form - 
//  form_state -
// Returns:
//
//
//-------------------------------------------------------------------------------
function ife_find_clicked_button($form, $form_state)
{
  return _ife_find_clicked_button($form, $form_state);
}
//-------------------------------------------------------------------------------
// _ife_find_clicked_button
// Params:
//  form - 
//  form_state -
// Returns:
//
//
//-------------------------------------------------------------------------------
function _ife_find_clicked_button(&$form, &$form_state)
{
  // Process Form Element
  if ((!empty($form['#type'])) && ($info = _element_info($form['#type']))) {
    $form += $info;
  }
  if (isset($form['#process']) && !$form['#processed']) {
    foreach ($form['#process'] as $process) {
      if (function_exists($process)) {
        $form = $process($form, isset($edit) ? $edit : NULL, $form_state, NULL);
      }
    }
  }
  // Check if its the Droid we are looking for.
  if(is_clicked_button($form)) {
    return $form;
  }
  // Repeat previous steps for children while letting them know who their parents are.
  foreach (element_children($form) as $key) {
    if (!isset($form[$key]['#tree'])) {
      $form[$key]['#tree'] = $form['#tree'];
    }
    if (!isset($form[$key]['#parents'])) {
      $form[$key]['#parents'] = $form[$key]['#tree'] && $form['#tree'] ? array_merge($form['#parents'], array($key)) : array($key);
      $array_parents = isset($form['#array_parents']) ? $form['#array_parents'] : array();
      $array_parents[] = $key;
      $form[$key]['#array_parents'] = $array_parents;
    }
    // Recurse through all the elements.
    $result = ife_find_clicked_button($form[$key], $form_state);
    if($result != NULL)
      return $result;
  }
  return NULL;
}
//-------------------------------------------------------------------------------
// ife_tabpanel_ahah
// Params: 
// Returns:
// 
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

  $clicked_button = ife_find_clicked_button($form, $form_state);
  
  switch($clicked_button['#action']) {
  case 'remove':
    ife_remove_tabpanel($form, $clicked_button);
    break;
  case 'add':
    ife_add_tabpanel($form, $clicked_button);
    break;
  default:
    exit();
  }

  $form_id = $form['#parameters'][0];
  $form['#post'] = $_POST;
  $form['#programmed']  = FALSE;
  form_set_cache($form_build_id, $form, $form_state);
  $form = form_builder($form_id, $form, $form_state);
  $output = ife_find_tabpanel_to_insert($form, $clicked_button);

  drupal_json(array(
		    'status'   => TRUE,
		    'data'     => theme('status_messages') . drupal_render($output),
		    //'data'     => theme('status_messages') . '<div>'.print_r($clicked_button, TRUE).'</div>',
		    ));
}
//-------------------------------------------------------------------------------
// theme_tabpanel
// Params:
//  element - form element of tabpanel type.
// Returns:
//  Themed html repersenting a collection of tabs.
// 
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
// theme_tabpanel_items
// Params:
//  form - The parent form element to all form elements within the tabpanel.
// Returns:
//  Themed html repersenting all the tabpanels and the elements contained within 
//  them.
// 
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