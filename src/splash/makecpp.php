#!/usr/bin/php -q

<?php
//cp splash.xpm splash.cpp

$dirs = array_filter(glob('*'), 'is_dir');

foreach ($dirs as $dir) {
  $inFileName = $dir."/splash.xpm";
  $outFileName = $dir."/splash.cpp";

  $inFile = fopen($inFileName, "r+");
  $outFile = fopen($outFileName, "w");
  $count = 0;
  while (!feof($inFile)) {    
     $buffer = fgets($inFile);
     $count++;
     $expr = "static char * splash_xpm[] = {";
     if ($expr == trim($buffer)) {
        fwrite($outFile,"extern SPLASH_EXPORT const char* splash_image[] = {\n");
     }
     else {
        fwrite($outFile,$buffer);
     }

  }    
  fclose($inFile);
  fclose($outFile);
}


/*
neu: extern SPLASH_EXPORT const char* splash_image[] = {
alt: static char * splash_xpm[] = {
*/

?>
