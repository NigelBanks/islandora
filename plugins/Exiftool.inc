<?php
// $Id$

/* File: Exiftool.inc
 * Author: 
 *  Chris Beer (Creator) 
 *  Nigel Banks (Re-factoring, Documentation, Integration for IslandVoices.ca)
 * 
 * Description: 
 *
 */

//-------------------------------------------------------------------------------
// say
// Params: 
//  a - Anything
// Returns: none.
//
// Debug function prints an object surrounded by div tags using print_r,
//-------------------------------------------------------------------------------
function say($a)
{
  echo '<div>'. print_r($a, true) .'</div>';
}

/******************************************************************************** 
 * Class: Exiftool
 *
 * Description: 
 *
 *******************************************************************************/
class Exiftool 
{
  private $pid = NULL;
  private $item = NULL;
  
  //-----------------------------------------------------------------------------
  // __construct
  // Params:
  //  pid - pid of the fedora object.
  //
  //-----------------------------------------------------------------------------
  function __construct($pid)
  {
    //drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
    $this->pid = $pid;
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    $this->item = new Fedora_Item($this->pid);
  }
  //-----------------------------------------------------------------------------
  // extractMetadata 
  // Params:
  //  parameterArray
  //  dsid - Datastream identifier
  //  file - 
  //  file_ext -
  // Returns: True always.
  // 
  //-----------------------------------------------------------------------------
  function extractMetadata($parameterArray, $dsid, $file, $file_ext) {
    $system = getenv('System');
    $file_suffix = '_'. $dsid . '.xml';
    $returnValue=TRUE;
    $output=array();
    exec('exiftool -X ' . escapeshellarg($file) . '', $output);
    file_put_contents($file.$file_suffix, implode("\n", $output));
    $_SESSION['fedora_ingest_files']["$dsid"] = $file . $file_suffix;
    return TRUE;
  }
  //-----------------------------------------------------------------------------
  // displayMetadata
  // Params:
  //  
  // Returns: metadata.
  // 
  //-----------------------------------------------------------------------------
  function displayMetadata() 
  { 
    $output='';
    $exifDom = DOMDocument::loadXML($this->item->get_datastream_dissemination('EXIF'));
    if ($exifDom != NULL) {
      $description = $exifDom->getElementsByTagNameNS('http://www.w3.org/1999/02/22-rdf-syntax-ns#','Description');
      if ($description->length > 0) { 
        $description=$description->item(0);
        $output .= '<div class="fedora_technical_metadata"><ul>';
        for ($i=0;$i<$description->childNodes->length;$i++) { 
          $name=$description->childNodes->item($i)->nodeName;
          $value=$description->childNodes->item($i)->nodeValue;
          if ($name != '#text'  && !preg_match('/^System\:.*$/',$name) && trim($value) != '') { 
            list($type,$name) = preg_split('/\:/',$name);
            $name = trim(preg_replace('/(?<!^)([A-Z][a-z]|(?<=[a-z])[A-Z])/'," $1", $name));
            $output .= '<li><b>'.$name. '</b>:  '. $value .' </li>';
          }
        }
        $output.='</ul></div>';
        
        $fieldset = array(
          '#title' => t("!text", array('!text' => 'Technical Metadata')),
          '#collapsible' => TRUE,
          '#collapsed' => TRUE,
          '#value' => $output);
        $output  = theme('fieldset', $fieldset);
      }
    }
    return $output;
  }
}
