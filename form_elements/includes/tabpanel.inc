
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
// ife_get_tabpanel_values
// Params:
//  clicked_button -
// Returns:
//
//
//-------------------------------------------------------------------------------
function &ife_get_tabpanel_values($clicked_button)
{
  $parents = $clicked_button['#parents'];  
  $depth = count($parents) - 2;
  $point = &$_POST;
  for($i = 0; $i < $depth; $i++) { 
    $point = &$point[$parents[$i]];
  }
  return $point;
}
//-------------------------------------------------------------------------------
// ife_get_tabpanel_content
// Params:
//  form - 
//  clicked_button -
// Returns:
//
//
//-------------------------------------------------------------------------------
function &ife_get_tabpanel_content(&$form, $clicked_button)
{
  $parents = $clicked_button['#parents'];
  $depth = count($parents) - 2; // The button element and the tab its under decreases the depth by 2
  $point = &$form;
  for($i = 0; $i < $depth; $i++) { 
    $point = &$point[$parents[$i]];
  }
  return $point;
}
//-------------------------------------------------------------------------------
// ife_count_tabpanels
// Params:
//  form - 
//  clicked_button -
// Returns:
//
//
//-------------------------------------------------------------------------------
function ife_count_tabpanels(&$content)
{
  $count = 1;
  while($content[$count])
    $count++;
  return $count;
}
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
  $content = &ife_get_tabpanel_content($form, $clicked_button);
  $count = ife_count_tabpanels($content);
  $index =  $clicked_button['#tab'];
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
  $content = ife_get_tabpanel_values($clicked_button);
  $count = count($content);
  $index =  $clicked_button['#tab'];
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
  _ife_remove_tabpanel_from_form($form, $clicked_button);
  _ife_remove_tabpanel_values($form, $clicked_button);
}
//-------------------------------------------------------------------------------
// ife_remove_added_tabpanels
// Params:
//  form -
//  clicked_button -
// Returns:
//
// 
//-------------------------------------------------------------------------------
function ife_remove_added_tabpanels(&$content)
{
  $count = ife_count_tabpanels($content);
  for($i = 1; $i < $count; $i++) {
    unset($content[$i]);
  }
}
//-------------------------------------------------------------------------------
// ife_prune_extra_tabpanels
// Params:
//  form -
//  clicked_button -
// Returns:
//
// 
//-------------------------------------------------------------------------------
function ife_prune_extra_tabpanels(&$tabpanel)
{
  $children = &$tabpanel['children']; 
  foreach (element_children($children) as $key) { // Find child tabpanels
    $type = $children[$key]['#type'];
    if(isset($children[$key]['content'])) {
      $content = &$children[$key]['content'];
      ife_remove_added_tabpanels($content);
      ife_prune_extra_tabpanels($content[0]);
    }
  }
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
  $content = &ife_get_tabpanel_content($form, $clicked_button);
  $count = ife_count_tabpanels($content);
  $content[$count] = $content[0];
  ife_prune_extra_tabpanels($content[$count]);
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
  $offset = ($clicked_button['#action'] == 'remove')? 3 : 3;
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

  unset($output['#prefix'], $ouput['#suffix']);
  $javascript = drupal_add_js(NULL, NULL, 'header');
  $settings = call_user_func_array('array_merge_recursive', $javascript['setting']);
  unset($settings['ahah']['']);
  drupal_json(array(
		    'status'   => TRUE,
		    'data'     => theme('status_messages') . drupal_render($output),
		    'settings' => $settings,
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
  drupal_add_js($path.'/js/jquery-ui-1.8.7.custom.min.js');
  drupal_add_js($path.'/js/tabpanel_ahah.js');
  drupal_add_css($path.'/css/ui-lightness/jquery-ui-1.8.7.custom.css');
  return '<div id=' . t(implode('-',$element['#array_parents'])) . ' class="action-expand-tab">'.$element['#children'].'</div>';
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
  $count = 1;
  $output .= '<ul>';
  $output .= '<li><a href="#' . $name . '-0">' . $title . '</a><span id="add-' . $name . '" class="ui-icon ui-icon-circle-plus" style="float: left; margin: 0.4em 0.2em 0 0; cursor: pointer;">'.$form[0]["add-tab"]['#id'].'</span></li>';
  $count = 1;
  while($form[$count]) {
    $output .= '<li><a href="#' . $name . '-'.$count.'">' . $count . '</a><span id="remove-' . $name . '-'. $count .'" class="ui-icon ui-icon-close" style="float: left; margin: 0.4em 0.2em 0 0; cursor: pointer;">'. $form[$count]['remove-tab']['#id'] .'</span></li>';
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
  $count = 1;
  return $output;
}