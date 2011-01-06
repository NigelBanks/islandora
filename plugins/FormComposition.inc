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
  // processFormComposition
  // Params:
  // Returns:
  //
  // Builds an in memory repersentation of the <form_composition> element.
  //-----------------------------------------------------------------------------
  public function constructFormElement($name)
  {
    $elementType = $this->getElementTypeByName($name);
    $form_element = $this->constructFormElementFromElementType($elementType, NULL); 
    $form_element_name = $this->getFormElementName($this->getAttribute('@name', $elementType));
    return array( $form_element_name, $form_element );
  }
  //-----------------------------------------------------------------------------
  private function getFormElementName($name)
  {
    if(!$name) return '0'; // In cases were a tabpanel or fieldset repersents a single element.
    $result = str_replace(array(':', '@'), '', $name);
    $result = str_replace(array('/'), '-', $result);
    return $result;
  }
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
  private function getAttribute($name, $domNode)
  {
    $attrNode = $this->getUniqueNode($name, $domNode);
    if($attrNode) return $attrNode->value;
    else return '';
  }
  //-----------------------------------------------------------------------------
  // processFormComposition
  // Params:
  // Returns:
  //
  // Builds an in memory repersentation of the <form_composition> element.
  //-----------------------------------------------------------------------------
  private function getElementTypeByName($name)
  {
    // TODO allow searching child elements for the particular elementType.
    $xpath = "/form_composition/elementTypes/elementType[@name='$name']";
    return $this->getUniqueNode($xpath);
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //-----------------------------------------------------------------------------
  private function getAttributeGroupByName($name)
  {
    $xpath = "/form_composition/attributeGroups/attributeGroup[@name='$name']";
    return $this->getUniqueNode($xpath);
  }
  //-----------------------------------------------------------------------------
  private function getDescription($element)
  {
    $description = $this->getUniqueNode("description", $element);
    if($description) return $description->textContent;
    else return '';
  }
  //-----------------------------------------------------------------------------
  private function getParentElementType($elementType)
  {
    $node = $this->getUniqueNode("../..", $elementType);
    if($node && nodeName == "elementType") return $node;
    else return NULL;
  }
  //-----------------------------------------------------------------------------
  private function getAttributes($elementType)
  {
    return $this->getUniqueNode("attributes", $elementType);
  }
  //-----------------------------------------------------------------------------
  private function getChildren($elementType)
  {
    return $this->getUniqueNode("children", $elementType);
  }
  //-----------------------------------------------------------------------------
  private function getOptions($element)
  {
    $options = $this->getUniqueNode("options", $element);
    if(!$options) return NULL;
    $result = $this->xpath->evaluate("option", $options);
    $values = NULL;
    for($i=0; $i < $result->length; $i++) {
      $values[$i] = $result->item($i)->textContent;
    }
    return $values;
  }
  //-----------------------------------------------------------------------------
  private function constructFormElementFromAttribute($attribute, $xpath)
  {
    $form_element['#title'] = $this->getAttribute("@label", $attribute);
    $form_element['#required'] = $this->getAttribute("@required", $attribute);
    $form_element['#type'] = $this->getAttribute("@type", $attribute);
    $form_element['#xpath'] = $xpath . '/' . $this->getAttribute('@name', $attribute);
    $form_element['#description'] = $this->getDescription($attribute);
    $form_element['#options'] = $this->getOptions($attribute);
    // Todo VALIDATION 
    return $form_element;
  }
  //-----------------------------------------------------------------------------
  private function constructFormElementFromElementTypeAttributes($elementType, $xpath)
  {
    $attributes = $this->getAttributes($elementType);
    if(!$attributes) return NULL;
    $form_element['#title'] = 'Attributes';
    $form_element['#type'] = 'fieldset';
    $form_element['#collapsible'] = TRUE;
    $form_element['#collapsed'] = TRUE;
    // Each Attribute
    $attributeList = $this->xpath->evaluate("attribute", $attributes);
    for($i = 0; $i < $attributeList->length; $i++) { 
      $attribute = $attributeList->item($i);
      $attributeName = $this->getAttribute("@name", $attribute);
      $formElementName = $this->getFormElementName($attributeName);
      $form_element[$formElementName] = $this->constructFormElementFromAttribute($attributeList->item($i), $xpath);
    }
    // Each Attribute Group Reference 
    $attributeGroupReferences = $this->xpath->evaluate("attributeGroupRef", $attributes);
    for($i = 0; $i < $attributeGroupReferences->length; $i++) { 
      $attributeGroupReferenceName = $attributeGroupReferences->item($i)->textContent;
      $attributeGroup = $this->getAttributeGroupByName($attributeGroupReferenceName);
      $this->constructFormElementsFromAttributeGroup($attributeGroup, $form_element, $xpath); 
    }
    return $form_element;
  }
  //-----------------------------------------------------------------------------
  private function constructFormElementsFromAttributeGroup($attributeGroup, &$form_element, $xpath)
  {
    // Each Attribute
    $attributeList = $this->xpath->evaluate("attribute", $attributeGroup);
    for($i = 0; $i < $attributeList->length; $i++) { 
      $attribute = $attributeList->item($i);
      $attributeName = $this->getAttribute("@name", $attribute);
      $formElementName = $this->getFormElementName($attributeName);
      $form_element[$formElementName] = $this->constructFormElementFromAttribute($attribute, $xpath);
    }
    // Each Attribute Group Reference 
    $attributeGroupReferences = $this->xpath->evaluate("attributeGroupRef", $attributeGroup);
    for($i = 0; $i < $attributeGroupReferences->length; $i++) { 
      $attributeGroupReferenceName = $attributeGroupReferences->item($i)->textContent;
      $attributeGroup = $this->getAttributeGroupByName($attributeGroupReferenceName);
      $this->constructFormElementsFromAttributeGroup($attributeGroup, $form_element, $xpath);
    }
  }
  //-----------------------------------------------------------------------------
  private function constructFormElementsFromElementTypeChildren($elementType, $xpath)
  {
    $children = $this->getChildren($elementType);
    if(!$children) return NULL;
    // Each ElementType
    $elementTypeList = $this->xpath->evaluate("elementType", $children);
    for($i = 0; $i < $elementTypeList->length; $i++) { 
      $elementType = $elementTypeList->item($i);
      $elementTypeName = $this->getAttribute("@name", $elementType);
      $formElementName = $this->getFormElementName($elementTypeName);
      $form_element[$formElementName] = $this->constructFormElementFromElementType($elementType, $xpath);
    }
    // Each ElementTypeRef
    $elementTypeRefList = $this->xpath->evaluate("elementTypeRef", $children);
    for($j = 0; $j < $elementTypeRefList->length; $j++) { 
      $elementTypeName = $elementTypeRefList->item($j)->textContent;
      $elementType = $this->getElementTypeByName($elementTypeName);
      $formElementName = $this->getFormElementName($elementTypeName);
      $form_element[$formElementName] = $this->constructFormElementFromElementType($elementType, $xpath);
    }
    return $form_element;
  }
  //-----------------------------------------------------------------------------
  private function getDefaultValue($element)
  {
    return NULL; // Implement
  }
  //-----------------------------------------------------------------------------
  // constructFormElementFromElementType
  // Params:
  // Returns:
  //
  //
  //-----------------------------------------------------------------------------
  private function constructFormElementFromElementType($elementType, $xpath = NULL)
  {  
    $name = $this->getAttribute('@name', $elementType);
    $xpath = ($xpath) ? $xpath . '/' . $name : $name;
    $form_element['#title'] = $this->getAttribute("@label", $elementType);
    $form_element['#required'] = $this->getAttribute("@required", $elementType);
    $form_element['#type'] = $this->getAttribute("@type", $elementType);
    $form_element['#description'] = $this->getDescription($elementType);
    $form_element['#xpath'] = $xpath;
    $form_element['#default_value'] = $this->getDefaultValue($elementType);
    $form_element['#options'] = $this->getOptions($elementType);
    $form_element['#tree'] = TRUE;
    if($form_element['#type'] == 'tabpanel' || $form_element['#type'] == 'fieldset') {
      $form_element['content'][0]['attributes'] = $this->constructFormElementFromElementTypeAttributes($elementType, $xpath);
      $form_element['content'][0]['children'] = $this->constructFormElementsFromElementTypeChildren($elementType, $xpath);
    }
    return $form_element;
  }
}
