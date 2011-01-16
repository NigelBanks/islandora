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
  protected static $GET_FORM_CONTROL_VALUE_FUNCTIONS = array(
    'xpath' => 'XPathFormControlValueFunction',
    'options' => 'OptionsFormControlValueFunction'
  );
  protected $fedoraItem;
  protected $doc;
  protected $xpath;

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
      $this->doc = DOMDocument::loadXML($content_model->getEditFormAsXMLString($name));
      $this->fedoraItem = new fedorae_item($object_pid);
    }
    else {
      $this->doc = DOMDocument::loadXML($content_model->getIngestFormAsXMLString($name));
    }
    $this->xpath = new DOMXPath($this->doc); // MODS.xml data access
    $this->xpath->registerNamespace("php", "http://php.net/xpath"); // Register the php: namespace (required)
  }

  /**
   * Build a drupal form.
   *
   * @return
   *   A drupal form repersenting the form.
   */
  public function buildForm() {
    $nodes = $this->mapNodeListToArray($this->xpath->evaluate("child::*"));
    return $this->buildFormElements($nodes);
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
    return $this->xpath->evaluate("count(child::*[name()='content'])", $node) == 1;
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
    if ($content = $this->xpath->evaluate("child::*[name()='content']", $node)->item(0)) {
      return array($this->buildFormElements($this->mapNodeListToArray($this->xpath->evaluate('child::*', $content))));
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
    $form_controls = $this->mapNodeListToArray($this->xpath->evaluate("child::*[name()!='content']", $node), 'tagName');
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
    if ($func = $this->getDrupalFormControlValueFunction($name)) {
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
    $count = $this->xpath->evaluate("count(child::*)", $node);
    if ($count == 0) {
      return array('#' . $name => $this->xpath->evaluate('child::text()', $node)->item(0)->textContent);
    }
    else { // Array of values
      return array('#' . $name => $this->mapNodeListToArray($this->xpath->evaluate('child::*/child::text()', $node), NULL, 'textContent'));
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
    $full_path = $this->xpath->evaluate("attribute::*[name()='full_path']", $node)->item(0)->value;
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
    $options = $this->mapNodeListToArray($this->xpath->evaluate('child::*/child::text()', $node), 'textContent', 'textContent');
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
