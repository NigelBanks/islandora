<?php

// $Id$

/* @file
 * 
 *  
 */
module_load_include('inc', 'fedora_repository', 'api/utils.inc');

/**
 * Short Description
 *
 * Long Description
 */
class FormComposition {
  const mods_ns = 'http://www.loc.gov/mods/v3';
  const xlink_ns = 'http://www.w3.org/1999/xlink';
  const xsi_ns = 'http://www.w3.org/2001/XMLSchema-instance';
  protected static $GET_FORM_CONTROL_VALUE_FUNCTIONS = array(
    'xpath' => 'XPathFormControlValueFunction',
    'options' => 'OptionsFormControlValueFunction'
  );
  protected $fedoraItem;
  protected $formDocument;
  protected $formDocumentXPath;
  protected $mods;
  protected $modsXPath;

  /**
   * Constructs a new FormComposition Object.
   *
   * @param $name
   *   The name of the form.
   * @param $content_model_pid
   *   The pid of the content model.
   * @param $object_pid
   *   The object pid.
   */
  function __construct($name, $content_model_pid, $object_pid = NULL) {
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    $content_model = ContentModel::loadFromModel($content_model_pid);
    if (isset($object_pid)) {
      $this->fedoraItem = new fedora_item($object_pid);
      $this->mods = new DOMDocument();
      $this->mods->loadXML($this->fedoraItem->get_datastream_dissemination('MODS'));
      $this->modsXPath = new DOMXPath($this->mods); // MODS.xml data access
      $this->modsXPath->registerNamespace("mods", self::mods_ns);
      $this->modsXPath->registerNamespace("xlink", self::xlink_ns);
      $this->modsXPath->registerNamespace("xsi", self::xsi_ns);
    }
    $this->formDocument = new DOMDocument();
    $this->formDocument->loadXML($content_model->getFormAsXMLString($name));
    $this->formDocumentXPath = new DOMXPath($this->formDocument); // MODS.xml data access
    $this->formDocumentXPath->registerNamespace("php", "http://php.net/xpath");
  }

  /**
   * Build a drupal form.
   *
   * @return
   *   A drupal form repersenting the form.
   */
  public function buildForm() {
    $nodes = $this->mapNodeListToArray($this->formDocumentXPath->evaluate("child::*"));
    $form = $this->buildFormElements($nodes);
    if (isset($this->mods)) {
      $parents = array();
      $this->populateFormValues($form, $form, $parents);
    }
    return $form;
  }

  /**
   *
   * @param <type> $form
   * @param <type> $root
   * @param <type> $parents
   */
  private function populateFormValues(&$form, &$root, &$parents) {
    foreach ($form as $index => &$element) {
      if (!is_numeric($index)) {
        continue;
      }
      if (isset($element['#xpath'])) {
        $xpath = $this->getXPath($element, $root, $parents);
        $this->populateFormValue($element, $xpath);
      }
      if ($element['content']) {
        array_unshift($parents, &$element);
        foreach ($element['content'] as $j => &$b) {
          if (!is_numeric($j)) {
            continue;
          }
          if ($element['#type'] == 'tabpanel') {
            $b['#xpath_index'] = $j + 1;
            array_unshift($parents, $b);
          }          
          $this->populateFormValues($b, $root, $parents);
          if($element['#type'] == 'tabpanel') {
            array_shift($parents);
          }
        }
        array_shift($parents);
      }
    }
  }

  /**
   *
   * @param array $element
   * @param string $xpath
   */
  private function populateFormValue(&$element, $xpath) {
    $result = $this->modsXPath->evaluate($xpath);
    if($result == FALSE) {
      $break_here;
      $break_here++;
    }
    $class = get_class($result);
    if (!isset($result) || ($class == 'DOMNodeList' && $result->length == 0)) {
      return;
    }
    switch ($element['#type']) {
      case 'tabpanel':
        for ($i = 1; $i < $result->length; $i++) {
          $element['content'][$i] = $element['content'][0];
        }
        break;
      case 'list':
        $value = $this->mapNodeListToArray($result, NULL, 'textContent');
        $element['#default_value'] = trim(implode(';', $value));
        break;
      default:
        $value = $result->item(0)->textContent;
        $element['#default_value'] = trim($value);
        break;
    }
  }

  /**
   *
   *
   */
  private function getXPath(&$form, &$root, &$parents) {
    if ($form['#xpath_full']) {
      return $form['#xpath'];
    }
    else {
      $xpath = $form['#xpath'];
      $count = count($parents);
      for ($i = 0; $i < $count; $i++) {
        $ancestor = $parents[$i];
        $parent_xpath = $ancestor['#xpath'];
        $parent_xpath_index = $ancestor['#xpath_index'];
        if (isset($parent_xpath)) {
          $xpath = (($xpath != '') && chr(ord($xpath)) != '[') ? '/' . $xpath : $xpath;
          $xpath = $parent_xpath . $xpath;
        }
        else if (isset($parent_xpath_index)) {
          $xpath = ($xpath != '') ? '/' . $xpath : $xpath;
          $xpath = "[$parent_xpath_index]" . $xpath;
        }
        if ($ancestor['#xpath_full']) {
          break;
        }
      }
      return $xpath;
    }
  }

  /**
   * Builds a drupal form, from an array of elements in the XML definition.
   *
   * @param $nodes
   *   An array of nodes within the form's XML definition, that repersent
   *   Drupal form elements.
   *
   * @return
   *   A drupal form repersenting the elements from the $nodes array.
   */
  private function buildFormElements($nodes) {
    $form = array();
    foreach ($nodes as $node) {
      $form[] = $this->buildFormElement($node);
    }
    return $form;
  }

  /**
   * Builds a drupal form, from an array of elements in the XML definition.
   *
   * @param $node
   *   A node from the form's XML definition, that repersent
   *   Drupal form element.
   *
   * @return
   *   A drupal form repersenting the element defined by $node.
   */
  private function buildFormElement(&$node) {
    $form = array('#type' => $node->tagName, '#tree' => TRUE);
    $this->setDrupalFormControls($form, $node);
    if ($this->hasContent($node)) {
      $form['content'] = $this->getContent($node);
    }
    return $form;
  }

  /**
   * Tests if a node has content.
   *
   * @param &$node
   *   A node from the form's XML definition, that repersent
   *   Drupal form element.
   *
   * @return
   *   Returns: TRUE if there exists a unique child named "content" in this 
   *   specified node parameter.
   */
  private function hasContent(&$node) {
    return $this->formDocumentXPath->evaluate("count(child::*[name()='content'])", $node) == 1;
  }

  /**
   * Gets content of node, and builds a drupal form element repersenting that
   * content.
   *
   * This function is only run on nodes that have a child 'content' so fieldsets
   * tabpanels and the like.
   *
   * @param &$node
   *   A node from the form's XML definition, that repersent
   *   Drupal form element.
   *
   * @return
   *   A drupal form repersenting all the elements within 'content'
   */
  private function getContent(&$node) {
    $content = $this->formDocumentXPath->evaluate("child::*[name()='content']", $node)->item(0);
    if ($content) {
      return array($this->buildFormElements($this->mapNodeListToArray($this->formDocumentXPath->evaluate('child::*', $content))));
    }
  }

  /**
   * Sets the form controls for a given node
   *
   * Form controls are things like #title.
   *
   * @param &$form
   *   Drupal form element whose controls will be set.
   * @param &$node
   *   A node from the form's XML definition, that repersent
   *   Drupal form element.
   */
  private function setDrupalFormControls(&$form, &$node) {
    $form_controls = $this->mapNodeListToArray($this->formDocumentXPath->evaluate("child::*[name()!='content']", $node), 'tagName');
    foreach ($form_controls as $name => $form_control_node) {
      $form = array_merge($form, $this->getDrupalFormControlValues($name, $form_control_node));
    }
  }

  /**
   * Gets a form control values.
   *
   * @param &$name
   *   The $node's tag name.
   * @param &$node
   *   A node from the form's XML definition, that repersents a form control.
   *
   * @return
   *   An array that has a collection of form controls. Example:
   * @code
   *   array('#title' => 'My Title', #options => array('option 1', 'option 2'))
   * @endcode
   */
  private function getDrupalFormControlValues(&$name, &$node) {
    $func = $this->getDrupalFormControlValueFunction($name);
    if ($func) {
      return $this->{$func}($name, $node);
    }
    return NULL;
  }

  /**
   * Gets a function name that can extract one or more form controls values
   * from a node in the form's XML definition.
   *
   * @param &$name
   *   The form controls name.
   *
   * @return
   *   A function name that can extract specific form controls.
   */
  private function getDrupalFormControlValueFunction(&$name) {
    if (isset(self::$GET_FORM_CONTROL_VALUE_FUNCTIONS[$name])) {
      return self::$GET_FORM_CONTROL_VALUE_FUNCTIONS[$name];
    }
    return 'DefaultFormControlValueFunction';
  }

  /**
   * Default function for extracting a form control from a node in
   * the form's XML definition.
   *
   * This can handle nodes that have only text content, or children
   * that only contain text content.
   *
   * @param &$name
   *   The tag name of $node.
   * @param &$node
   *   A DomNode from the form's XML definition that repersent a form control.
   * @return
   *   A form control.
   */
  private function DefaultFormControlValueFunction(&$name, &$node) {
    $count = $this->formDocumentXPath->evaluate("count(child::*)", $node);
    if ($count == 0) {
      $result = $this->formDocumentXPath->evaluate('child::text()', $node);
      if($result !== FALSE && $result->length > 0) {
        return array('#' . $name => $result->item(0)->textContent);
      }
      else {
        return array('#' . $name => NULL);
      }
    }
    else { // Array of values
      return array('#' . $name => $this->mapNodeListToArray($this->formDocumentXPath->evaluate('child::*/child::text()', $node), NULL, 'textContent'));
    }
  }

  /**
   * Extracts form controls for xpath elements in the form's XML definition.
   *
   * @param &$name
   *   The tag name of $node.
   * @param &$node
   *   A DomNode from the form's XML definition that repersent a form control.
   * @return
   *   Two form controls #xpath and #xpath_full. #xpath_full is a boolean 
   *   that describes if #xpath is a full xpath or just a part of one.
   */
  private function XPathFormControlValueFunction(&$name, &$node) {
    $full_path = $this->formDocumentXPath->evaluate("attribute::*[name()='full_path']", $node)->item(0)->value;
    $full_path = strcasecmp($full_path, 'true') == 0 ? TRUE : FALSE;
    $xpath_form_control = $this->DefaultFormControlValueFunction($name, $node);
    if ($xpath_form_control['#xpath'] == NULL) {
      $xpath_form_control['#xpath'] = '';
    }
    return array_merge($xpath_form_control, array('#xpath_full' => $full_path));
  }

  /**
   *
   * @param string $name
   * @param DomNode $node 
   */
  private function OptionsFormControlValueFunction(&$name, &$node) {
    $options = $this->mapNodeListToArray($this->formDocumentXPath->evaluate('child::*/child::text()', $node), 'textContent', 'textContent');
    if ($node->getAttribute('ignoreFirstOption') == 'true') {
      $select_message = array_shift($options);
      $options = array_merge(array(NULL => $select_message), $options);
    }
    return array('#options' => $options);
  }

  /**
   * Helper function that maps a node list to an array.
   *
   * Supports maping the keys and values of the returned array to values within
   * each node.
   *
   * @param &$node_list
   *   A DomNodeList.
   * @param $key
   *   (optional) A string that sets the keys of the array to this class 
   *   property of the DomNode. If not set the array elements will be numbered.
   * @param $value
   *   (optional) A string that sets the values of the array to this class 
   *   property of the DomNode. If not set the value will be the DOMNode.
   *
   * @return
   *   An mapped array of keys and values, determined by the parameters $key and $value.
   */
  private function mapNodeListToArray($node_list, $key = NULL, $value = NULL) {
    $result = NULL;
    $count = $node_list->length;
    for ($i = 0; $i < $count; $i++) {
      $node = $node_list->item($i);
      if ($key) {
        if ($value) {
          $result[$node->$key] = $node->$value;
        }
        else {
          $result[$node->$key] = $node;
        }
      }
      else {
        if ($value) {
          $result[] = $node->$value;
        }
        else {
          $result[] = $node;
        }
      }
    }
    return $result;
  }

}
