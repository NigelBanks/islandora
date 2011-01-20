<?php

// $Id$

/**
 * @file
 */

/**
 *
 */
class FormTree {

  protected $contentModel;

  /**
   *
   * @global String $base_url
   * @param ContentModel $content_model
   */
  public function __construct($content_model) {
    global $base_url;
    $this->moduleRoot = $base_url . '/' . drupal_get_path('module', 'islandora_content_modeller');
    $this->contentModel = $content_model;
  }

  /**
   *
   * @param String $model_pid
   * @return FormTree
   */
  public static function createFromPID($model_pid) {
    $content_model = ContentModel::loadFromModel($model_pid);
    return new FormTree($content_model);
  }

  /**
   *
   * @param <type> $icon
   * @param <type> $is_form
   * @param <type> $function
   * @param <type> $params
   * @return <type>
   */
  function icon($icon, $is_form, $function, $params) {
    $class = ($is_form) ? 'ajaxFormIcon' : 'ajaxButtonIcon';
    $arguments = implode(' ', $params);
    return "<img src='" . $this->moduleRoot . "/images/$icon' class='$class' id='$function $arguments'/>" . " ";
  }

  function getFormIcons($form_name) {
    $params = array($this->contentModel->pid, $form_name);
    $icons = icon('remove.png', FALSE, 'icm_model_remove_form', $params);
    $icons .= icon('add.png', TRUE, 'icm_model_add_element_to_form', $params);
    $icons .= icon('blue_up.png', FALSE, 'icm_model_inc_form_element', $params);
    $icons .= icon('blue_down.png', FALSE, 'icm_model_dec_form_element', $params);
    $icons .= icon('edit.gif', TRUE, 'icm_model_edit_form', $params);
    return $icons;
  }

  function getFormElementIcons($form_name, $form_element) {
    $path_from_form_node = array_reverse($this->getPathToFormElement($form_element));
    $params = array($this->contentModel->pid, $form_name, implode(' ', $path_from_form_node));
    $icons = icon('remove.png', FALSE, 'icm_model_remove_form_element', $params);
    $icons .= icon('add.png', TRUE, 'icm_model_add_form_element', $params);
    $icons .= icon('blue_up.png', FALSE, 'icm_model_add_form_element', $params);
    $icons .= icon('blue_down.png', FALSE, 'icm_model_add_form_element', $params);
    $icons .= icon('edit.gif', TRUE, 'icm_model_edit_form_element', $params);
    return $icons;
  }

  public function attachFormsToTree($model_tree) {
    $icons = $this->icon('add.png', TRUE, 'icm_model_add_form', array($this->contentModel->pid));
    $forms_tree_node = $model_tree->addChild('Forms', 'folder', $icons);
    $this->attachEachFormToTree($forms_tree_node);
  }

  private function attachEachFormToTree($model_tree) {
    $form_names = $this->contentModel->getFormNames();
    foreach ($form_names as $name) {
      $icons = $this->getFormIcons($form_name);
      $form_tree_node = $model_tree->addChild("<b>Form: $name</b> ", 'folder', $icons);
      $form = $this->contentModel->getFormElement($name);
      $this->attachEachFormElementToTree($name, $form, $form_tree_node);
    }
  }

  private function attachEachFormElementToTree($form_name, $form_element, $model_tree) {
    $this->forEachChildElementDo($form_element, 'attachFormElementToTree', array($form_name, $model_tree));
  }

  private function attachFormElementToTree($child, $params) {
    $form_name = $params[0];
    $model_tree = $params[1];
    $name = $child->tagName;
    $title = $child->getElementsByTagName('title')->item(0)->textContent;
    $icons = $this->getFormElementIcons($form_name, $child);
    $tree_node = $model_tree->addChild("<b>$name, ($title)</b> ", 'folder', $icons);
    $content = $child->getElementsByTagName('content');
    if ($content && $content->length > 0) {
      $this->attachEachFormElementToTree($form_name, $content->item(0), $tree_node);
    }
  }

  private function forEachChildElementDo($element, $function, $params = NULL) {
    if (!method_exists($this, $function))
      return FALSE;
    $children = $element->childNodes;
    for ($i = 0; $i < $children->length; $i++) {
      $child = $children->item($i);
      if (get_class($child) != 'DOMElement') {
        continue;
      }
      call_user_func(array($this, $function), $child, $params);
    }
  }

  private function findChildIndex($child, $params) {
    $form_element = &$params[0];
    $count = &$params[1];
    $found = &$params[2];
    if (!$found) {
      if ($child->isSameNode($form_element)) {
        $found = TRUE;
      }
      else {
        $count++;
      }
    }
  }

  private function getIndexOfChildInParent($form_element) {
    $count = 0;
    $found = FALSE;
    $this->forEachChildElementDo($form_element->parentNode, 'findChildIndex', array(&$form_element, &$count, &$found));
    return $count;
  }

  private function getPathToFormElement($form_element) {    
    $path[] = $this->getIndexOfChildInParent($form_element);
    if ($form_element->parentNode->tagName != 'form') {
      $path = array_merge($path, $this->getPathToFormElement($form_element->parentNode));
    }
    return $path;
  }
}