<?php
// $Id$

/* File: Ffmpeg.inc
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
class Ffmpeg 
{
  //-----------------------------------------------------------------------------
  // ShowQtStreamsInFieldSets
  // Params:
  //  pid - pid of the fedora object.
  // Returns: none.
  //
  // Nigel thinks this should be a constructor.
  //-----------------------------------------------------------------------------
  function Ffmpeg() 
  {
  }
  //-----------------------------------------------------------------------------
  // ShowQtStreamsInFieldSets
  // Params:
  //  pid - pid of the fedora object.
  // Returns: none.
  //
  //
  //----------------------------------------------------------------------------- 
  function extract_thumbnail($parameterArray, $dsid, $file, $file_ext) 
  {
    $defaults = array('ss' => '00:00:10', 's' => null); 
    $params = array_merge($defaults, $parameterArray);
    $system = getenv('System');
    $file_suffix = '_'. $dsid . '.' . $file_ext;
    $returnValue=TRUE;
    $output=array();
    $size = '';    
    if($params['s'] != null) {
      $size = ' -s ' . escapeshellarg($params['s']);
    }
    exec('ffmpeg -i ' . escapeshellarg($file) . ' -r 1 -ss ' . escapeshellarg($params['ss']) . ' ' . $size . ' -t 1 ' . escapeshellarg($file . $file_suffix));

    if (!file_exists($file . $file_suffix)) {
      return false;
    }
    $_SESSION['fedora_ingest_files']["$dsid"] = $file . $file_suffix;
    return TRUE;
  }
}
