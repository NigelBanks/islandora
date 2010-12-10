<?php
// $Id$

/* File: qt_viewer.inc
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
 * Class: ShowQtStreamsInFieldSets
 *
 * Description: 
 *
 *******************************************************************************/
class ShowQtStreamsInFieldSets 
{
  private $pid =NULL;
  //-----------------------------------------------------------------------------
  // ShowQtStreamsInFieldSets
  // Params:
  //  pid - pid of the fedora object.
  // Returns: none.
  //
  // Nigel thinks this should be a constructor.
  //-----------------------------------------------------------------------------
  function ShowQtStreamsInFieldSets($pid) 
  {
    $this->pid = $pid;
  }
  //-----------------------------------------------------------------------------
  // fedoraObject
  // Params: none.
  // Returns: a fedora_item identified by the pid passed to ShowQtStreamsInFieldSets.
  //
  // return a fedora_item object identifed by the pid passed to ShowQtStreamsInFieldSets
  //-----------------------------------------------------------------------------
  function fedoraObject() 
  {
    return new fedora_item($this->pid);
  }
  //-----------------------------------------------------------------------------
  // technicalMetadata
  // Params:
  //  defaults - An array ...
  //  dsid - Datastream identifier. 'OBJ_EXIFTOOL' by default.
  // Returns: ? metadata
  //
  // Description goes here.
  //-----------------------------------------------------------------------------
  function technicalMetadata($defaults = array(), $dsid = 'OBJ_EXIFTOOL') 
  {
    $data = $defaults;
    try {
      $src = ObjectHelper::getStream($this->pid, $dsid);
      $doc = new SimpleXMLElement($src);
      $doc->registerXPathNamespace('File', 'http://ns.exiftool.ca/File/1.0/');
      $doc->registerXPathNamespace('Composite', 'http://ns.exiftool.ca/Composite/1.0/');
      $mime = reset($doc->xpath('//File:MIMEType'));
      $data['mime'] = $mime;
      if(strpos($mime, 'audio/') !== false) {
        $data['width'] = 300;
        $data['height'] = 0;
      } 
      else {
        $size = reset($doc->xpath('//Composite:ImageSize/text()'));
        list($width, $height) = explode('x', $size);
        $data['width'] = $width;
        $data['height'] = $height;
      }
      $data['doc'] = $src;
    } 
    catch(Exception $e) {
      $data = $defaults;
    }
    return $data;
  }
  //-----------------------------------------------------------------------------
  // getPosterFrameDatastreamInfo
  // Params:
  //  dsid - Datastream identifer. 'FULL_SIZE' by default.
  // Returns: A datastream information object if dsid is valid, otherwise nil.
  //
  // Helper method to quickly get information about a datastream.
  //-----------------------------------------------------------------------------
  function getPosterFrameDatastreamInfo($dsid = 'FULL_SIZE') 
  {
    $p = ObjectHelper::getDatastreamInfo($this->pid, $dsid);
    if(empty($p) || $p == ' ' || $p === false) {
      return false;
    }
    return $p;
  }
  //-----------------------------------------------------------------------------
  // getMediaDatastreamInfo
  // Params:
  //  dsid - Datastream identifer, 'OBJ' by default.
  // Returns:
  // 
  // Description goes here.
  //-----------------------------------------------------------------------------
  function getMediaDatastreamInfo($dsid = 'OBJ', $alt = array('')) 
  {
    $p = ObjectHelper::getDatastreamInfo($this->pid, $dsid);
    if(empty($p) || $p == ' ' || $p === false) {
      if(!empty($alt)) {
        $ds = array_shift($alt);
        return $this->getMediaDatastreamInfo($ds, $alt);
      }
      return false;
    }
    return $p;
  }
  //-----------------------------------------------------------------------------
  // enableDownload
  // Params: none.
  // Returns: false.
  //
  // Looks like a stubbed function, that would be used to determine if the user
  // has the rights to download a particular datastream.
  //-----------------------------------------------------------------------------
  function enableDownload() 
  {
    return false;
  }
  //-----------------------------------------------------------------------------
  // showQt
  // Params: none.
  // Returns: 
  //
  // Shows a Quick Time film?
  //-----------------------------------------------------------------------------
  function showQt() 
  {
    module_load_include('inc', 'fedora_repository', 'plugins/tagging_form');
    module_load_include('inc', 'fedora_repository', 'plugins/ShowStreamsInFieldSets');
    module_load_include('inc', 'fedora_repository', 'api/fedora_item');
    
    $info = $this->technicalMetadata(array('width' => 640, 'height' => 480));
    $width = $info['width'];
    $height = $info['height'];

    $pframe = $this->getPosterFrameDatastreamInfo();
    $media = $this->getMediaDatastreamInfo('PROXY', array('OBJ'));
    if($media === false ) {
      return '';
    }
    global $base_url;
    $path = drupal_get_path('module', 'Fedora_Repository');
    $fullPath=base_path().$path;
    $content= '';
    $pathTojs = drupal_get_path('module', 'Fedora_Repository').'/js/AC_Quicktime.js';
    drupal_add_js($pathTojs);

    $divid = 'player'.md5($this->pid).'MOV';
    $content .= '<div  class="player"  id="' . $divid .'">';
    if($pframe !== false) {
      $content .= '<div class="poster" style="cursor: pointer; position: relative; width: ' . $width .'px; min-height: ' . ($height) . 'px;">';
      $content .= '<img src="' . base_path().'fedora/repository/'.$this->pid.'/'. $pframe->ID . '/poster.jpg' . '" />';
      $content .= '<div class="play" style="font-size: 128px; color: white; position: absolute; top: 50%; left: 50%; margin-top: -0.085em; margin-left: -0.33em; opacity: 0.9; ">▶</div>';
      $content .= '</div>';
    }
    $content .= '</div>';
    if($this->enableDownload()) {
      $url = base_path().'fedora/repository/'.$this->pid.'/OBJ/MOV.mov';
      $content .= '<a class="download" href="' . $url . '">Download Media File</a>';
    }
    $src = base_path().'fedora/repository/'.$this->pid.'/' . $media->ID. '/MOV.mov';
    $qtparams = '';
    $qtparams .= "'autostart', '" . ($pframe !== false ? 'true' : 'false') . "', ";
    $init = <<<EOD
      $(function() { 
          src = "$src";
          if(src.substring(0,4) != 'http') {
            src = 'http://' + location.host + src;
          }
          str = QT_GenerateOBJECTText_XHTML(src, "$width", ($height+15), '', 
                $qtparams
                'postdomevents', 'true',
                'EnableJavaScript', 'true',
                'bgcolor', 'black',
                'controller', 'true',
                'SCALE', 'aspect',
                'LOOP', 'false');		
          if($('.poster', '#$divid').length ==0) {
            $('#$divid').append(str);
          } 
          else {
            $('#$divid .poster').one('click', function() { $(this).hide(); $('#$divid').append(str); });
          }
        });
EOD;
    
    drupal_add_js($init, 'inline', 'footer');
    
    $collection_fieldset = array(
      '#title' => t('Quicktime'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
      '#value' => $content);
    return theme('fieldset',$collection_fieldset);
  }
}