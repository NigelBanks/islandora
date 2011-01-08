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
 *******************************************************************************/
class FormComposition
{
  static $MODS_NS = 'http://www.loc.gov/mods/v3';
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
  function __construct($contentModelPid, $objectPid=NULL) 
  {
    if ($contentModelPid == null)
      throw new Exception('FormComposition constructor, requires a valid pid for the contentModel.');
    module_load_include('inc', 'fedora_repository', 'ContentModel');
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');        
    $this->pid = $pid;
    $this->contentModel = ContentModel::loadFromModel($contentModelPid);
    if($objectPid !== null)
      $this->fedoraItem = new fedorae_item($objectPid);
    $this->doc = DOMDocument::loadXML($this->contentModel->getFormCompositionAsXMLString());
    $this->xpath = new DOMXPath($this->doc); // MODS.xml data access
    $this->xpath->registerNamespace("php", "http://php.net/xpath"); // Register the php: namespace (required)
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  //
  // 
  //-----------------------------------------------------------------------------
  public function getFormElement($name)
  {
    $elementTypeNode = $this->getElementTypeNode($name);
    return $this->getElementType($elementTypeNode);
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  //
  // 
  //-----------------------------------------------------------------------------
  private function evaluate($xpath, $context, $value = NULL)
  {
    $result = $this->xpath->evaluate($xpath, $context);
    if(!$result || is_string($result)) {
      return $result;
    }
    $count = $result->length;
    for($i = 0; $i < $count; $i++) {
      $node = $result->item($i);
      if($value) {
	$array[] = $node->$value;
      }
      else {
	$array[] = $node;
      }
    }
    return $array;
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  //
  // 
  //-----------------------------------------------------------------------------
  private function mapNodesTo($key, $value, $xpath, $context)
  {
    $nodeArray = $this->evaluate($xpath, $context);
    if(is_array($nodeArray)) {
      $count = count($nodeArray);
      foreach($nodeArray as $node) {
	$array[$node->$key] = $node->$value;
      }
    }
    return $array;
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  //
  // 
  //-----------------------------------------------------------------------------
  private function formatNameForFormElement($name)
  {
    $result = trim($name);
    $result = str_replace(array('/', '@'), '', $result);
    $length = strlen($result);
    $index = strpos($result, ':');
    if($index !== FALSE)
      $result = substr($result, $index+1, $length - $index -1);
    return $result;
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  //
  // 
  //-----------------------------------------------------------------------------
  private function getUniqueNode($xpath, $domNode = NULL)
  {
    $result = FALSE;
    if($domNode)
      $result = $this->xpath->evaluate($xpath, $domNode);
    else
      $result = $this->xpath->evaluate($xpath);
    if($result == FALSE || $result->length != 1) { // Doesn't exist or isn't unique
      return NULL;
    }
    return $result->item(0);
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  //
  // 
  //-----------------------------------------------------------------------------
  private function getElementTypeNode($name, $context = NULL)
  {
    if(!$context) {
      $xpath = "/form_composition/elementTypes/elementType[@name='$name']";
      return $this->getUniqueNode($xpath);
    }
    else {
      $xpath = "/children/elementType[@name='$name']";
      return $this->getUniqueNode($xpath, $context);
    }
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  //
  // 
  //-----------------------------------------------------------------------------
  private function getDescription($element)
  {
    $description = $this->getUniqueNode("description", $element);
    if($description) return $description->textContent;
    else return '';
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  //
  // 
  //-----------------------------------------------------------------------------
  private function getOptions(&$node)
  {
    if(!$optionsNode = $this->getUniqueNode("options", $node)) {
      return NULL;
    }
    return $this->evaluate('option', $optionsNode, 'textContent');
  }
  //-----------------------------------------------------------------------------
  // 
  // Params:
  // Returns:
  //
  //
  //-----------------------------------------------------------------------------
  private function getElementType(&$node)
  {  
    list($name, $form) = $this->createForm($node);
    if($this->hasContent($form)) {
      $form['content'][0] = $this->getContent($node);
    }
    return array($name => $form);
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //
  //-----------------------------------------------------------------------------
  private function createForm(&$node)
  {
    $attributes = $this->mapNodesTo('name', 'value', '@*', $node);
    $name = $this->formatNameForFormElement($attributes["name"]);
    return array($name, array(
	          '#title' => $attributes["label"],
		  '#required' => $attributes["required"],
		  '#type' => $attributes["type"],
		  '#description' => $this->getDescription($node),
		  '#default_value' => $attributes["value"],
		  '#options' => $this->getOptions($node),
		  '#tree' => TRUE,
		  '#form_composition_attributes' => $attributes,
    ));
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //
  //-----------------------------------------------------------------------------
  private function hasContent(&$form)
  {
    $type = $form['#type'];
    return $type == 'tabpanel' || $type == 'fieldset';
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //
  //----------------------------------------------------------------------------
  private function getContent(&$node) 
  {
    $childrenNode = $this->getUniqueNode('children', $node);
    return array(
		 '#form_composition_attribute' => $this->mapNodesTo('name', 'value', '@*', $childrenNode),
		 'attributes' => $this->getAttributes($node),
		 'children' => $this->getChildren($node),
		 );
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //
  //---------------------------------------------------------------------------
  private function getAttributes(&$node)
  {
    if(!$attributesNode = $this->getUniqueNode('attributes', $node))
      return NULL;
    $form = array(
		  '#title' => 'Attributes',
		  '#type' => 'fieldset',
		  '#collapsible' => TRUE,
		  '#collapsed' => TRUE,
		  );
    $form = array_merge($form, $this->getEachAttribute($attributesNode));
    return array_merge($form, $this->getEachAttributeFromGroupReference($attributesNode));
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //
  //-----------------------------------------------------------------------------
  private function getEachAttribute(&$node)
  {
    $form = array();
    if(!$attributeNodes = $this->evaluate('attribute', $node)) {
      return $form;
    }
    foreach($attributeNodes as $attributeNode) {
      list($name, $value) = $this->createForm($attributeNode);
      $form[$name] = $value;
    }
    return $form;
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //
  //-----------------------------------------------------------------------------
  private function getEachAttributeFromGroupReference(&$node)
  {
    $form = array();
    if(!$attributeGroupReferences = $this->evaluate('attributeGroupRef', $node, 'textContent')) {
      return $form;
    }
    foreach($attributeGroupReferences as $name) {
      $xpath = "/form_composition/attributeGroups/attributeGroup[@name='$name']";
      $attributeGroupNode = $this->getUniqueNode($xpath);
      $form = array_merge($form, $this->getEachAttribute($attributeGroupNode));
      $form = array_merge($form, $this->getEachAttributeFromGroupReference($attributeGroupNode));
    }
    return $form;
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //
  //-----------------------------------------------------------------------------
  private function getChildren(&$elementTypeNode)
  {
    if(!$childrenNode = $this->getUniqueNode('children', $elementTypeNode))
      return NULL;
    $form = $this->getEachElementType($childrenNode);
    return array_merge($form, $this->getEachElementTypeRef($childrenNode));
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //
  //-----------------------------------------------------------------------------
  private function getEachElementType(&$childrenNode)
  {
    $form = array();
    if(!$elementTypeNodes = $this->evaluate("elementType", $childrenNode)) {
      return $form;
    }
    foreach($elementTypeNodes as $elementTypeNode) {
      $form = array_merge($form, $this->getElementType($elementTypeNode));
    }
    return $form;
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //
  //-----------------------------------------------------------------------------
  private function getEachElementTypeRef(&$childrenNode)
  {
    $form = array();
    if(!$elementTypeReferences = $this->evaluate('elementTypeRef', $childrenNode, 'textContent')) {
      return $form;
    }
    foreach($elementTypeReferences as $name) {
      $xpath = "/form_composition/elementTypes/elementType[@name='$name']";
      $elementTypeNode = $this->getUniqueNode($xpath);
      $form = array_merge($form, $this->getElementType($elementTypeNode));
    }
    return $form;
  }
}
