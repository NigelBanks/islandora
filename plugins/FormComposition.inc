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
  private function getUniqueNode($name, $element)
  {
    $result = $this->xpath->evaluate($name, $element);
    if($result == FALSE || $result->length != 1 ) // Doesn't exist or isn't unique
      return FALSE;
    return $result->item(0);
  }
  //-----------------------------------------------------------------------------
  private function getAttribute($name, $element)
  {
    $node = $this->getUniqueNode($name, $element);
    if($node) return $node->value;
    else return '';
  }
  //-----------------------------------------------------------------------------
  private function getFormElementName($name, $parentNodeName)
  {
    if(is_string($parentNodeName) && $parentNodeName != '')
      return $parentNodeName . '/' . $name;
    else
      return $name;
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
    $result = $this->xpath->evaluate("/form_composition/elementTypes/elementType[@name='$name']");
    if($result == FALSE || $result->length != 1)  
      throw new Exception('Specifed a elementType that does not exist or is not unique within the Content Model.');
    return $result->item(0);
  }
  //-----------------------------------------------------------------------------
  //
  // Params:
  // Returns:
  //
  //
  //-----------------------------------------------------------------------------
  private function getAttributeGroupByName($name)
  {
    $result = $this->xpath->evaluate("/form_composition/attributeGroups/attributeGroup[@name='$name']");
    if($result == FALSE || $result->length != 1)  
      throw new Exception('Specifed a AttributeGroupRef that does not exist or is not unique within the Content Model.');
    return $result->item(0);
  }
  //-----------------------------------------------------------------------------
  // processFormComposition
  // Params:
  // Returns:
  //
  // Builds an in memory repersentation of the <form_composition> element.
  //-----------------------------------------------------------------------------
  public function constructFormElement($name, $parentNodeName='')
  {
    $elementType = $this->getElementTypeByName($name);
    $form_element = $this->constructFormElementFromElementType($elementType, $parentNodeName); 
    $form_element_name = $this->getFormElementName($this->getAttribute("@name", $elementType), $parentNodeName);
    return array( $form_element_name => $form_element );
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
  private function constructFormElementFromAttributes($attributes, $parentNodeName)
  {
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
      $form_element[$this->getFormElementName($attributeName, $parentNodeName)] = $this->constructFormElementFromAttribute($attributeList->item($i));
    }
    // Each Attribute Group Reference 
    $attributeGroupReferences = $this->xpath->evaluate("attributeGroupRef", $attributes);
    for($i = 0; $i < $attributeGroupReferences->length; $i++) { 
      $attributeGroupReferenceName = $attributeGroupReferences->item($i)->textContent;
      $attributeGroup = $this->getAttributeGroupByName($attributeGroupReferenceName);
      $this->constructFormElementsFromAttributeGroup($attributeGroup, $form_element, $parentNodeName); 
    }
    return $form_element;
  }
  //-----------------------------------------------------------------------------
  private function constructFormElementFromAttribute($attribute)
  {
    // Attributes
    $form_element['#title'] = $this->getAttribute("@label", $attribute);
    $form_element['#required'] = $this->getAttribute("@required", $attribute);
    $form_element['#type'] = $this->getAttribute("@type", $attribute);
    // Description
    $form_element['#description'] = $this->getDescription($attribute);
    // Todo VALIDATION 
    $form_element['#options'] = $this->getOptions($attribute);
    return $form_element;
  }
  //-----------------------------------------------------------------------------
  private function constructFormElementsFromAttributeGroup($attributeGroup, &$form_element, $parentNodeName)
  {
    // Each Attribute
    $attributeList = $this->xpath->evaluate("attribute", $attributeGroup);
    for($i = 0; $i < $attributeList->length; $i++) { 
      $attribute = $attributeList->item($i);
      $attributeName = $this->getAttribute("@name", $attribute);
      $form_element[$this->getFormElementName($attributeName, $parentNodeName)] = $this->constructFormElementFromAttribute($attribute);
    }
    // Each Attribute Group Reference 
    $attributeGroupReferences = $this->xpath->evaluate("attributeGroupRef", $attributeGroup);
    for($i = 0; $i < $attributeGroupReferences->length; $i++) { 
      $attributeGroupReferenceName = $attributeGroupReferences->item($i)->textContent;
      $attributeGroup = $this->getAttributeGroupByName($attributeGroupReferenceName);
      $this->constructFormElementsFromAttributeGroup($attributeGroup, $form_element, $parentNodeName);
    }
  }
  //-----------------------------------------------------------------------------
  private function constructFormElementsFromChildren($children, $parentNodeName)
  {
    if(!$children) return NULL;
    // Each ElementType
    $elementTypeList = $this->xpath->evaluate("elementType", $children);
    for($i = 0; $i < $elementTypeList->length; $i++) { 
      $elementType = $elementTypeList->item($i);
      $elementTypeName = $this->getAttribute("@name", $elementType);
      $form_element[$this->getFormElementName($elementTypeName, $parentNodeName)] = $this->constructFormElementFromElementType($elementType, $parentNodeName);
    }
    // Each ElementTypeRef
    $elementTypeRefList = $this->xpath->evaluate("elementTypeRef", $children);
    for($i = 0; $i < $elementTypeRefList->length; $i++) { 
      $elementTypeName = $elementTypeRefList->item($i)->textContent;
      $elementType = $this->getElementTypeByName($elementTypeName);
      $form_element[$this->getFormElementName($elementTypeName, $parentNodeName)] = $this->constructFormElementFromElementType($elementType, $parentNodeName);
    }
    return $form_element;
  }
  //-----------------------------------------------------------------------------
  private function getDefaultValue($element)
  {
    if($form_element['#type'] == 'fieldset') return NULL;
  }
  //-----------------------------------------------------------------------------
  private function constructFormElementFromElementType($elementType, $parentNodeName)
  {  
    $elementTypeName = $this->getAttribute("@name", $elementType);
    $parentNodeName = $this->getFormElementName($elementTypeName, $parentNodeName); 
    // Attributes
    $form_element['#title'] = $this->getAttribute("@label", $elementType);
    $form_element['#required'] = $this->getAttribute("@required", $elementType);
    $form_element['#type'] = $this->getAttribute("@type", $elementType);
    // Description
    $form_element['#description'] = $this->getDescription($elementType);
    // Attributes
    $form_element['attributes'] = $this->constructFormElementFromAttributes($this->getAttributes($elementType), $parentNodeName);
    // Children?
    $form_element['children'] = $this->constructFormElementsFromChildren($this->getChildren($elementType), $parentNodeName);
    // Default Value?
    $form_element['#default_value'] = $this->getDefaultValue($elementType);
    // TODO. Validation
    $form_element['#options'] = $this->getOptions($elementType);
    return $form_element;
  }
}
