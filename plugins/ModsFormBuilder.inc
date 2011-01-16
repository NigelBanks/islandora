
<?php

// $Id$

/* @file
 *
 *  Generates a drupal form for editing an existing MODS data stream as well as
 *  a form that creates a MODS data stream on ingest.
 */
module_load_include('inc', 'fedora_repository', 'api/utils.inc');
module_load_include('inc', 'fedora_repository', 'plugins/FormComposition');
module_load_include('inc', 'fedora_repository', 'plugins/FormBuilder');

//-------------------------------------------------------------------------------
// say
// Params: 
//  a - Anything
// Returns: none.
//
// Debug function prints an object surrounded by div tags using print_r, remove after finished!
//-------------------------------------------------------------------------------
function say($a) {
  if (is_bool($a)) {
    $a = $a ? 'true' : 'false';
  }
  echo '<div>' . print_r($a, true) . '</div>';
}

/**
 * Repersents a MODS XML document.
 *
 * Extents DOMDocument
 */
class ModsDocument extends DOMDocument {
  const mods_ns = 'http://www.loc.gov/mods/v3';
  const xlink_ns = 'http://www.w3.org/1999/xlink';
  const xsi_ns = 'http://www.w3.org/2001/XMLSchema-instance';
  const root = 'root';
  private $formValues;

  /**
   * Construct a MODS XML document.
   *
   * @param &$form_values
   *   Submited form values in which the MODS document is generated from.
   */
  function __construct(&$form_values) {
    parent::__construct();
    $this->formatOutput = TRUE;
    $this->xpath = new DOMXPath($this);
    $this->xpath->registerNamespace("mods", self::mods_ns);
    $this->xpath->registerNamespace("xlink", self::xlink_ns);
    $this->xpath->registerNamespace("xsi", self::xsi_ns);
    $this->formValues = $form_values;
    $this->form = &$this->getForm();
    $this->createModsDocumentFromForm();
  }

  /**
   * Gets the cached drupal form.
   *
   * This form will be used for retrieving form controls such as #xpath,
   * to aid in the construction of the MODS XML document. 
   *
   * @return
   *  Cached drupal form.
   */
  private function &getForm() {
    $form_state = array('storage' => NULL, 'submitted' => FALSE, 'post' => $_POST);
    $form_build_id = $this->formValues['form_build_id'];
    if (!$form = form_get_cache($form_build_id, $form_state))
      exit();
    $this->setInternalFormControls($form[self::root], $form_state);
    return $form;
  }

  /**
   *
   * @param <type> $form
   */
  private function setInternalFormControls(&$form, &$form_state) {
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
      $this->setInternalFormControls($form[$key], $form_state);
    }
  }

  /**
   * Creates and populates the MODS XML document nodes with the submitted form 
   * values.
   */
  private function createModsDocumentFromForm() {
    $form_values = &$this->formValues[self::root];
    $form = &$this->form[self::root];
    $this->recurseElements($form, $form_values);
    $this->setRootAttributes();
    say("Testing");
    say(($this->saveXML()));
    exit();
  }

  /**
   * 
   */
  private function setRootAttributes() {
    $root = $this->documentElement;
    $root->setAttribute('xmlns:xlink', self::xlink_ns);
    $root->setAttribute('xmlns:mods', self::mods_ns);
    $root->setAttribute('xmlns:xsi', self::xsi_ns);
    $this->setCorrectVersion($root);
  }

  /**
   * 
   */
  private function setCorrectVersion($root) {
    $version = $root->getAttribute('version');
    switch ($version) {
      case '3.0':
        $root->setAttribute('version', '3.0');
        $root->setAttribute('xsi:schemaLocation', self::mods_ns . " http://www.loc.gov/standards/mods/v3/mods-3-0.xsd");
        break;
      case '3.1':
        $root->setAttribute('version', '3.1');
        $root->setAttribute('xsi:schemaLocation', self::mods_ns . " http://www.loc.gov/standards/mods/v3/mods-3-1.xsd");
        break;
      case '3.2':
        $root->setAttribute('version', '3.2');
        $root->setAttribute('xsi:schemaLocation', self::mods_ns . " http://www.loc.gov/standards/mods/v3/mods-3-2.xsd");
        break;
      case '3.3':
        $root->setAttribute('version', '3.3');
        $root->setAttribute('xsi:schemaLocation', self::mods_ns . " http://www.loc.gov/standards/mods/v3/mods-3-3.xsd");
        break;
      default:
        $root->setAttribute('version', '3.4');
        $root->setAttribute('xsi:schemaLocation', self::mods_ns . " http://www.loc.gov/standards/mods/v3/mods-3-4.xsd");
        break;
    }
  }

  /**
   * Description
   *
   * @param $parentElement
   * @param $form_element
   * @param $content
   *
   */
  private function recurseElements($form, $form_values) {
    foreach ($form as $index => $item) {
      if (is_numeric($index)) {
        $form_value = $form_values[$index];
        if (isset($item['#xpath']) && isset($form_value) && !is_array($form_value)) {
          $path = $this->getXPath($item);
          if ($path) {
            var_dump($path);
            var_dump($form_value);
            echo "<br/>";
            $this->createOrModifyElement($path, $item, $form_value);
          }
        }
        if ($this->hasContent($item)) {
          $this->processContent($item['content'], $form_value['content']);
        }
      }
    }
    return;
  }

  /**
   * Check if a form has content. Fieldsets and tabpanels have content.
   */
  private function hasContent(&$form) {
    return isset($form['content']);
  }

  /**
   * Check if a form has content. Fieldsets and tabpanels have content.
   */
  private function processContent(&$form, &$form_values) {
    foreach ($form as $index => $item) {
      if (is_numeric($index)) {
        $this->recurseElements($item, $form_values[$index]);
      }
    }
  }

  /**
   * 
   *
   */
  private function createOrModifyElement($path, $form, $form_value) {
    $node = $this->getNodeFromXPath($path);
    $class = get_class($node);
    switch ($class) {
      case 'DOMElement':
        $node->appendChild($this->createTextNode($form_value));
        break;
      case 'DOMAttr':
        $node->value = $form_value;
        break;
    }
  }

  /**
   *
   */
  private function getNodeFromXPath($path) {
    $result = $this->xpath->evaluate($path);
    if(!$result) {
      drupal_set_message(t('XPath: %xpath generated by the form is not valid please correct this in the content modeller.', array('%xpath' => $path)));
      return;
    }
    $class = get_class($result);
    switch ($class) {
      case 'DOMNodeList':
        $found = $result->length == 1;
        if ($found) {
          return $result->item(0);
        }
        return $this->createDomNodeFromXPath($path);
      default:
        return $result;
    }
  }

  /**
   *
   */
  private function createDomNodeFromXPath($path) {
    $parent_path = $this->getParentPathFromXPath($path);
    if (strlen($parent_path) == 0) { // No elements exist we've stumbled the root parent that doesn't exist.
      return $this->createDomNode($path, $this);
    }
    else {
      $result = $this->xpath->evaluate($parent_path);
      if (isset($result) && $result->length == 1) {
        return $this->createDomNode($path, $result->item(0));
      }
      else {
        $parent_node = $this->createDomNodeFromXPath($parent_path);
        return $this->createDomNode($path, $parent_node);
      }
    }
  }

  /**
   *
   */
  private function createDomNode($path, $parent_node) {
    if ($this->isXPathToAttribute($path)) {
      $name = $this->getAttributeNameFromXPath($path);
      $node = $this->createAttribute($name);
      $parent_node->setAttributeNode($node);
      return $node;
    }
    else {
      $name = $this->getElementNameFromXPath($path);
      $node = $this->createElementNS(self::mods_ns, $name);
      $parent_node->appendChild($node);
      return $node;
    }
  }

  /**
   *
   */
  private function getParentPathFromXPath($path) {
    $index = strrpos($path, '/');
    return substr($path, 0, $index);
  }

  /**
   *
   */
  private function getAttributeNameFromXPath($path) {
    $index = strrpos($path, '@');
    $len = strlen($path);
    return substr($path, $index + 1, $len - 1);
  }

  /**
   *
   */
  private function getElementNameFromXPath($path) {
    $index = strrpos($path, '/');
    $len = strlen($path);
    $name = substr($path, $index + 1, $len - 1);
    if (($index = strrpos($name, '[')) !== FALSE) {
      $name = substr($name, 0, $index);
    }
    return $name;
  }

  /**
   *
   */
  private function isXPathToAttribute($path) {
    return strpos($path, '@') !== FALSE;
  }

  /**
   *
   */
  private function bar($parents) {
    $point = &$this->form[self::root];
    foreach ($parents as $ancestor) {
      $point = &$point[$ancestor];
    }
    return $point;
  }

  /**
   * 
   *
   */
  private function getXPath($form) {
    if ($form['#xpath_full']) {
      return $form['#xpath'];
    }
    else {
      $xpath = $form['#xpath'];
      $parents = $form['#array_parents'];
      $count = count($parents);
      for ($i = 0; $i < $count; $i++) {
        array_pop($parents);
        $ancestor = $this->bar($parents);
        $parent_xpath = $ancestor['#xpath'];
        $parent_xpath_index = $ancestor['#xpath_index'];
        if (isset($parent_xpath)) {
          $parent_xpath = $parent_xpath . (isset($parent_xpath_index) ? '[' . $parent_xpath_index . ']' : '');
          $xpath = $parent_xpath . (($xpath != '') ? '/' . $xpath : '');
          if ($ancestor['#xpath_full']) {
            break;
          }
        }
      }
      return $xpath;
    }
  }

}

/**
 * Builds/Handles form's that repersent MODS documents
 *
 * Handles content model callbacks to build and handle submitted forms. 
 * Ultimately this class is resposible for ingesting/editing datastreams via
 * its MODS edit/ingest forms.
 */
class ModsFormBuilder extends FormBuilder {

  protected $contentModel;
  protected $fedoraItem;
  protected $pid;

  /**
   * @param $pid
   *   (optional) Object pid, of the object that will be edited.
   */
  function __construct($pid = NULL) {
    parent::__construct();
    if ($pid !== NULL) { // Editing a data stream
      module_load_include('inc', 'fedora_repository', 'ContentModel');
      module_load_include('inc', 'fedora_repository', 'api/fedora_item');
      $this->pid = $pid;
      $this->contentModel = ContentModel::loadFromObject($pid);
      $this->fedoraItem = new fedora_item($pid);
    }
  }

  /**
   * Test if a data stream is managed or not.
   *
   * @param $dsid
   *   Data stream identifier.
   * 
   * @return
   *  TRUE if the data stream specified by $disd is managed, FALSE otherwise.
   */
  private function isDatastreamManaged($dsid) {
    $datastream = $this->fedoraItem->get_datastream($dsid);
    return $datastream->controlGroup == 'M';
  }

  /**
   * Overwrites a managed data stream by reference.
   *
   * Only works for managed data streams. Creates a temp file to be used as the 
   * reference
   *
   * @param $dom
   *   XML document that will replace the data stream.
   * @param $dsid
   *   Data stream identifier.
   * @param $label
   *   Data stream label.
   * @param $mime_type
   *   Data stream mime_type. 
   */
  private function modifyManagedDatastream($dom, $dsid, $label, $mime_type) {
    $temp_file_dir = file_directory_temp();
    $temp_file_path = file_create_filename("modsdatastream.xml", $temp_file_dir);
    if ($dom->save($temp_file_path)) {
      $temp_file_url = file_create_url($temp_file_path);
      if ($this->fedoraItem->modify_datastream_by_reference($temp_file_url, $dsid, $label, $mime_type) !== NULL) {
        drupal_set_message(t('Successfully updated %dsid datastream for object %pid', array('%pid' => $this->pid, '%dsid' => $dsid)));
      }
    }
  }

  /**
   * Overwrites a inlined data stream by value.
   *
   * Only works for inlined data streams.
   *
   * @param $dom
   *   XML document that will replace the data stream.
   * @param $dsid
   *   Data stream identifier.
   * @param $label
   *   Data stream label.
   * @param $mime_type
   *   Data stream mime_type. 
   */
  private function modifyInlineDatastream($dom, $dsid, $label, $mime_type) {
    if ($this->fedoraItem->modify_datastream_by_value($dom->saveXML(), $dsid, $label, $mime_type) !== NULL) {
      drupal_set_message(t('Successfully updated %dsid datastream for object %pid', array('%pid' => $this->pid, '%dsid' => $dsid)));
    }
  }

  /**
   * Overwrites a data stream.
   *
   * Overwrites the given datastream by value for inline datastreams and by
   * reference for managed datastreams.
   *
   * @param $dom
   *   XML document that will replace the data stream.
   * @param $dsid
   *   Data stream identifier.
   * @param $label
   *   Data stream label.
   * @param $mime_type
   *   Data stream mime_type. 
   */
  private function modifyDatastream($dom, $dsid, $label, $mime_type) {
    if ($this->isDatastreamManaged($dsid)) {
      $this->modifyManagedDatastream($dom, $dsid, $label, $mime_type);
    }
    else {
      $this->modifyInlineDatastream($dom, $dsid, $label, $mime_type);
    }
  }

  /**
   * Process the submited Edit form, modifying the associated data stream.
   *
   * This function receives the submitted form, converts it into a MODS 
   * XML document and overwrites the existing MODS data stream with it.
   * It also transforms the MODS XML document into a Dublin Core XML document,
   * That is used to overwrite the existing DC data stream. Afterwards the user is 
   * redirected to the edited object.
   *
   * @param &$form_id
   *   Not used.
   * @param &$form_values
   *   Submitted form values.
   * @param &$soap_client
   *   Not used.
   */
  public function handleEditMetadataForm(&$form_id, &$form_values, &$soap_client) {
    $mods = new ModsDocument($form_values); // TODO: We need to pass the Object pid as well for merging.
    say($mods->saveXML());
    exit();
    $this->modifyDatastream($mods, 'MODS', 'MODS Record', 'text/xml');
    $dublin_core = new DomDocument("1.0", "UTF-8");
    $dublin_core->formatOutput = TRUE;
    $dublin_core->appendChild($this->transformModsToDublinCore($mods->saveXML(), $dublin_core));
    $this->modifyDatastream($dublin_core, 'DC', "Dublin Core Record", 'text/xml');
    drupal_goto('/fedora/repository/' . $this->pid);
  }

  /**
   * Creates an drupal form repersenting a MODS XML document.
   *
   * @param &$form
   *   Form to attach the generated form to.
   * @param $elements
   *   Not used.
   * @param &$form_values
   *   Not used.
   * 
   * @return 
   *   A drupal form repersenting a MODS XML document.
   */
  public function buildIngestForm(&$form, $elements, &$form_values) {
    $form['#multistep'] = TRUE; // used so that it triggers a form rebuild every time.
    $form_composition = new FormComposition('full', $form['content_model_pid']['#value']);
    $form['root'] = $form_composition->buildForm();
    $form['submit'] = array(
      '#type' => 'submit',
      '#submit' => array('fedora_repository_edit_qdc_form_submit'),
      '#value' => 'Save Metadata');
    $form['#cache'] = TRUE;
    $form['#tree'] = TRUE;
    return $form;
  }

  /**
   * Handles the submission of the ingest form generated by buildIngestForm
   *
   * Creates FOXML document representing the submitted form values, and 
   * ingests the FOXML document. 
   *
   * @param &$form_values
   *   Submitted form values.
   */
  function handleIngestForm(&$form_values) {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    module_load_include('inc', 'fedora_repository', 'CollectionPolicy');
    $foxml_document = $this->createFoxmlDocument($form_values);
    $this->ingestFoxmlDocument($foxml_document, $form_values['ingest-file-location']);
  }

  /**
   * Creates a foxml document based on submitted form values.
   *
   * Creates foxml document to be ingested. Creates and populates data streams
   * based on the values submitted in the form.
   *
   * @param &$form_values
   *   Submitted form values.
   *
   * @return 
   *   FOXML document.
   */
  private function createFoxmlDocument(&$form_values) {
    $foxml_document = new DomDocument("1.0", "UTF-8");
    $foxml_document->formatOutput = TRUE;
    $pid = $form_values['pid'];
    $root = $foxml_document->createElement("foxml:digitalObject");
    $attributes = array(
      'VERSION' => '1.1',
      'PID' => "$pid",
      'xmlns:foxml' => 'info:fedora/fedora-system:def/foxml#',
      'xmlns:xsi' => 'http://www.w3.org/2001/XMLSchema-instance',
      'xsi:schemaLocation' => 'info:fedora/fedora-system:def/foxml# http://www.fedora.info/definitions/1/0/foxml1-1.xsd');
    foreach ($attributes as $key => $val) {
      $root->setAttribute($key, $val);
    }
    $foxml_document->appendChild($root);

    // Add default values as specified by FormBuilder.inc
    $form_values['dc:title'] = $form_values['mods_title'];
    $this->createStandardFedoraStuff($form_values, $foxml_document, $root);
    $this->createRelationShips($form_values, $foxml_document, $root);

    $collection_pid = $form_values['collection_pid'];
    // TODO: Is this necessary, maybe its used in the parent class?
    if (($collection_policy = CollectionPolicy::LoadFromCollection($collection_pid)) !== FALSE) {
      $name = trim($collection_policy->getName());
      if (trim($name) != '') {
        $form_values['dc_relation'] = $name;
      }
    }
    // End TODO 
    if (!empty($form_values['ingest-file-location'])) {
      $this->createFedoraDataStreams($form_values, $foxml_document, $root);
    }

    // Done within this class.
    $this->createPolicy($collection_pid, $foxml_document, $root);
    $this->createModsStream($form_values, $foxml_document, $root);
    $this->createCollectionPolicy($form_values, $foxml_document, $root);
    $this->createWorkflowStream($form_values, $foxml_document, $root);
    return $foxml_document;
  }

  /**
   * Ingests a FOXML document, and file.
   *
   * @param &$foxml_document
   *   FOXML document.
   * @param $ingest_file_location
   *   File to be ingested.
   *
   * @todo The ingest file does not get deleted on failure to ingest. 
   *   Check to see if this is the desired behaviour?
   */
  function ingestFoxmlDocument(&$foxml_document, $ingest_file_location) {
    try {
      $object = Fedora_Item::ingest_from_FOXML($foxml_document);
      if (!empty($object->pid)) {
        drupal_set_message(t("Item !pid created successfully.", array('!pid' => l($object->pid, 'fedora/repository/' . $object->pid))), "status");
      }
      if (!empty($_SESSION['fedora_ingest_files'])) {
        foreach ($_SESSION['fedora_ingest_files'] as $dsid => $created_file) {
          file_delete($created_file);
        }
      }
      file_delete($ingest_file_location);
    } catch (exception $e) {
      drupal_set_message(t('Error ingesting object: !e', array('!e' => $e->getMessage())), 'error');
      watchdog(t("Fedora_Repository"), t("Error ingesting object: !e", array('!e' => $e->getMessage())), NULL, WATCHDOG_ERROR);
      return;
    }
  }

  /**
   * Creates an Edit form.
   *
   * Generates a drupal form that repersents MODS XML document, auto populating
   * the forms values with info stored in the MODS datastream for this object.
   *
   * @return
   *   The constructed Drupal form.
   */
  function buildEditMetadataForm() {
    if (!isset($this->contentModel)) {
      return NULL;
    }
    $form['#tree'] = TRUE;
    $form['#multistep'] = TRUE;
    $form['indicator2'] = array('#type' => 'fieldset', '#title' => t('Edit Meta-data'));
    $form['pid'] = array('#type' => 'hidden', '#value' => $this->pid);
    $form_composition = new FormComposition('full', $this->contentModel->pid, $this->pid);
    $form['indicator2'] = array_merge($form['indicator2'], $form_composition->buildForm());
    $form['submit'] = array(
      '#type' => 'submit',
      '#submit' => array('fedora_repository_edit_qdc_form_submit'),
      '#value' => 'Save Metadata');
    return $form;
  }

  /**
   * Creates a FOXML repersentation of data stream with the specified
   * attributes.
   *
   * @param $foxml_document 
   *   FOXML document to be ingested
   * @param $attributes
   *   FOXML attributes for the data stream
   * @param $version_attributes
   *   FOXML attributes for the version element of the data stream
   *
   * @return
   *   An array where the first item is the newly created data stream node,
   *   and the second item is the content node.
   */
  private function createDatastream(&$foxml_document, $attributes, $version_attributes) {
    $datastream = $foxml_document->createElement("foxml:datastream");
    foreach ($attributes as $key => $val) {
      $datastream->setAttribute($key, $val);
    }
    $version = $foxml_document->createElement('foxml:datastreamVersion');
    foreach ($version_attributes as $key => $val) {
      $version->setAttribute($key, $val);
    }
    $datastream->appendChild($version);
    $content = $foxml_document->createElement("foxml:xmlContent");
    $version->appendChild($content);
    return array(&$datastream, &$content);
  }

  /**
   * Creates Collection policy data stream from a template stored within the
   * Content Model, and appends it to the FOXML documents root element.
   *
   * @param $form_values
   *   Submitted form values.
   * @param $foxml_document 
   *   FOXML document to be ingested.
   * @param $root
   *   FOXML document root element.
   */
  private function createCollectionPolicy($form_values, &$foxml_document, &$root) {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    $fedora_item = new fedora_item($form_values['content_model_pid']);
    $datastreams = $fedora_item->get_datastreams_list_as_array();
    if (isset($datastreams['COLLECTION_POLICY_TMPL'])) {
      $collection_policy_template = $fedora_item->get_datastream_dissemination('COLLECTION_POLICY_TMPL');
      $collection_policy_template_dom = DOMDocument::loadXML($collection_policy_template);
      $collection_policy_template_root = $collection_policy_template_dom->getElementsByTagName('collection_policy');
      if ($collection_policy_template_root->length > 0) {
        $collection_policy_template_root = $collection_policy_template_root->item(0);
        $new_node = $foxml_document->importNode($collection_policy_template_root, TRUE);
        $attributes = array('ID' => 'COLLECTION_POLICY', 'STATE' => 'A', 'CONTROL_GROUP' => 'X');
        $version_attributes = array('ID' => 'COLLECTION_POLICY.0', 'LABEL' => 'Collection Policy', 'MIMETYPE' => 'text/xml');
        list($datastream, $content) = createDatastream($foxml_document, $attributes, $version_attributes);
        $content->appendChild($new_node);
        $root->appendChild($datastream);
      }
    }
  }

  /**
   * Creates WorkFlow datastream from a template stored within the Content Model,
   * and appends it to the FOXML documents root element.
   *
   * @param $form_values
   *   Submitted form values.
   * @param &$foxml_document 
   *   FOXML document to be ingested.
   * @param $root
   *   FOXML document root element.
   */
  private function createWorkflowStream($form_values, &$foxml_document, &$root) {
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    $fedora_item = new fedora_item($form_values['content_model_pid']);
    $datastreams = $fedora_item->get_datastreams_list_as_array();
    if (isset($datastreams['WORKFLOW_TMPL'])) {
      $work_flow_template = $fedora_item->get_datastream_dissemination('WORKFLOW_TMPL');
      $work_flow_template_dom = DOMDocument::loadXML($work_flow_template);
      $work_flow_template_root = $work_flow_template_dom->getElementsByTagName('workflow');
      if ($work_flow_template_root->length > 0) {
        $work_flow_template_root = $work_flow_template_root->item(0);
        $new_node = $foxml_document->importNode($work_flow_template_root, TRUE);
        $attributes = array('ID' => 'WORKFLOW', 'STATE' => 'A', 'CONTROL_GROUP' => 'X');
        $version_attributes = array('ID' => 'MODS.0', 'LABEL' => 'MODS Record', 'MIMETYPE' => 'text/xml');
        list($datastream, $content) = createDatastream($foxml_document, $attributes, $version_attributes);
        $content->appendChild($new_node);
        $root->appendChild($datastream);
      }
    }
  }

  /**
   * Creates a MODS data stream from the values in the submitted form, 
   * and appends it to the FOXML document. Also creates a Dublin Core 
   * data stream based on the MODS datastream, and appends it to the 
   * FOXML Document. 
   *
   * @param $form_values
   *   Submitted form values.
   * @param &$foxml_document 
   *   FOXML document to be ingested.
   * @param $root
   *   FOXML document root element.
   */
  private function createModsStream($form_values, &$foxml_document, &$root) {
    $attributes = array('ID' => 'MODS', 'STATE' => 'A', 'CONTROL_GROUP' => 'X');
    $version_attributes = array('ID' => 'MODS.0', 'LABEL' => 'MODS Record', 'MIMETYPE' => 'text/xml');
    list($datastream, $content) = $this->createDatastream($foxml_document, $attributes, $version_attributes);
    $mods = new ModsDocument($form_values);
    say(htmlentities($mods->saveXML()));
    exit();
    $node = $foxml_document->importNode($mods->documentElement, TRUE);
    $content->appendChild($node);
    $root->appendChild($datastream);
    $this->createDublinCoreDatastream($mods, $foxml_document, $root);
  }

  /**
   * Creates a Dublin Core document from a MODS document, and append it
   * to the FOXML document.
   *
   * @param $mods
   *   ModsDocument to be transformed.
   * @param &$foxml_document 
   *   FOXML document to be ingested.
   * @param $root
   *   FOXML document root element.
   */
  private function createDublinCoreDatastream(&$mods, &$foxml_document, &$root) {
    $attributes = array('ID' => 'DC', 'STATE' => 'A', 'CONTROL_GROUP' => 'X');
    $version_attributes = array('ID' => 'DC.0', 'LABEL' => 'Dublin Core Record', 'MIMETYPE' => 'text/xml');
    list($datastream, $content) = $this->createDatastream($foxml_document, $attributes, $version_attributes);
    $dublin_core_node = $this->transformModsToDublinCore($mods, $foxml_document);
    $content->appendChild($dublin_core_node);
    $root->appendChild($datastream);
  }

  /**
   * Transforms a MODS document into a Dublin Core Document via XSLT.
   * 
   * @param $mods
   *   ModsDocument to be transformed.
   * @param &$foxml_document 
   *   FOXML document to be ingested.
   * @param $root
   *   FOXML document root element.
   *
   * @return 
   *   A DOMDocumentFragment that contains the transformed Dublin Core
   *   Document.
   */
  private function transformModsToDublinCore(&$mods, &$foxml_document) {
    // TODO: Clean up this ugly function.
    $doc = new DOMDocument();
    $doc->loadXML($mods->saveXML());
    $xsl = new DOMDocument();
    $xsl->load(drupal_get_path('module', 'fedora_repository') . '/workflow_client/xsl/mods_to_dc.xsl');
    $proc = new XSLTProcessor();
    $proc->importStyleSheet($xsl);
    $xml = $proc->transformToXML($doc);
    $doc->loadXML($xml);
    $dublin_core_document = $foxml_document->createDocumentFragment();
    $test = $doc->saveXML($doc->firstChild);
    $dublin_core_document->appendXML($doc->saveXML($doc->firstChild));
    return $dublin_core_document;
  }

}