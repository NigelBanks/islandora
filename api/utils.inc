<?php

// $Id$

/* File: utils.inc
 * 
 * Description: 
 *  A place for general use functions that can be used more or less anywhere.
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
