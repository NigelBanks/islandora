<?php

// $Id$

/* File: FormComposition.inc
 * Author: 
 *  Nigel Banks 
 * 
 * Description: 
 *  Repersents the <form_composition> element of a content model
 *  providing convience methods for processing and building elements from
 *  the schema.
 */
module_load_include('inc', 'fedora_repository', 'api/utils.inc');

/******************************************************************************** 
 * Class: FormComposition
 *
 * Description: 
 *
 * Should really be named form builder...
 *******************************************************************************/
class FormComposition
{
  static $MODS_NS = 'http://www.loc.gov/mods/v3';
  protected static $GET_FORM_CONTROL_VALUE_FUNCTIONS = array(
							     'xpath' => 'XPathFormControlValueFunction',
							     ); 
  protected $contentModel;
  protected $fedoraItem;
  protected $pid; 
  protected $doc;
  protected $xpath;

  //-----------------------------------------------------------------------------
  // Constructor
  // Params: 
  //  pid - Object ID of the Content Model
  //-----------------------------------------------------------------------------
  function __construct($contentModelPid, $name, $objectPid=NULL) 
  {
    if ($contentModelPid == null)
      throw new Exception('FormComposition constructor, requires a valid pid for the contentModel.');
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');        
    $this->pid = $pid;
    $this->contentModel = ContentModel::loadFromModel($contentModelPid);
    if($objectPid !== null) {
      $this->doc = DOMDocument::loadXML($this->contentModel->getEditFormAsXMLString($name));      
      $this->fedoraItem = new fedorae_item($objectPid);
    }
    else {
      $this->doc = DOMDocument::loadXML($this->contentModel->getIngestFormAsXMLString($name));
    }
    $this->xpath = new DOMXPath($this->doc); // MODS.xml data access
    $this->xpath->registerNamespace("php", "http://php.net/xpath"); // Register the php: namespace (required)
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  // 
  //-----------------------------------------------------------------------------
  public function buildForm()
  {
    $nodes = $this->mapNodeListToArray($this->xpath->evaluate("child::*"));
    return $this->buildFormElements($nodes);
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  // 
  //-----------------------------------------------------------------------------
  private function buildFormElements($nodes) 
  {
    $form = array();
    foreach($nodes as $node) {
      $form[] = $this->buildFormElement($node);
    }
    return $form;
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  // 
  //-----------------------------------------------------------------------------
  private function buildFormElement(&$node)
  {
    $form = array('#type' => $node->tagName);
    $this->setDrupalFormControls($form, $node);
    if($this->hasContent($node)) {
      $form['content'] = $this->getContent($node);
    }
    return $form;
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns: TRUE if there exists a unique child named "content" in this 
  //  specified node parameter;  
  // 
  //-----------------------------------------------------------------------------
  private function hasContent(&$node) 
  {
    return $this->xpath->evaluate("count(child::*[name()='content'])", $node) == 1;
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  // 
  //-----------------------------------------------------------------------------
  private function getContent(&$node) 
  {
    if($content = $this->xpath->evaluate("child::*[name()='content']", $node)->item(0)) {
      return array($this->buildFormElements($this->mapNodeListToArray($this->xpath->evaluate('child::*', $content))));
    }
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  // 
  //-----------------------------------------------------------------------------
  private function setDrupalFormControls(&$form, &$node) 
  {
    $formControls = $this->mapNodeListToArray($this->xpath->evaluate("child::*[name()!='content']", $node), 'tagName');
    foreach($formControls as $name => $formControlNode) {
      $form = array_merge($form,  $this->getDrupalFormControlValues($name, $formControlNode));
    }
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  // 
  //-----------------------------------------------------------------------------
  private function getDrupalFormControlValues(&$name, &$node) 
  {
     if($func = $this->getDrupalFormControlValueFunction($name)) {
       return $this->{$func}($name, $node);
     }
     return NULL;
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  // 
  //-----------------------------------------------------------------------------
  private function getDrupalFormControlValueFunction(&$name) 
  {
    if(isset(self::$GET_FORM_CONTROL_VALUE_FUNCTIONS[$name])) {
      return self::$GET_FORM_CONTROL_VALUE_FUNCTIONS[$name];
    }
    return 'DefaultFormControlValueFunction';
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  // 
  //-----------------------------------------------------------------------------
  private function DefaultFormControlValueFunction(&$name, &$node)
  {
    $count = $this->xpath->evaluate("count(child::*)", $node);
    if($count == 0) {
      return array('#'. $name => $this->xpath->evaluate('child::text()', $node)->item(0)->textContent);
    }
    else { // Array of values
      return array('#'.$name => $this->mapNodeListToArray($this->xpath->evaluate('child::*/child::text()', $node), NULL, 'textContent'));
    }
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  // 
  //-----------------------------------------------------------------------------
  private function XPathFormControlValueFunction(&$name, &$node)
  {
    $full_path = $this->xpath->evaluate("attribute::*[name()='full_path']", $node)->item(0)->value;
    $full_path = strcasecmp($full_path, 'true') == 0 ? TRUE : FALSE;
    $defaultXPathDrupalFormControlValue = $this->DefaultFormControlValueFunction($name, $node);
    return array_merge($defaultXPathDrupalFormControlValue, array('#xpath_full' => $full_path));
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  // 
  //-----------------------------------------------------------------------------
  private function mapNodeListToArray($nodeList, $key = NULL, $value = NULL) 
  {
    $result = NULL;
    $count = $nodeList->length;
    for($i = 0; $i < $count; $i++) {
      $node = $nodeList->item($i);
      if($key) {
	if($value) {
	  $result[$node->$key] = $node->$value;
	}
	else {
	  $result[$node->$key] = $node;
	}
      }
      else {
	if($value) {
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
