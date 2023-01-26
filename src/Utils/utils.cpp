#include "utils.h"

bool Utils::validateIPv6( QString IP )
{
  bool ok;

  if ( IP.count(":") < 2)
     return false;

  // Validate the V6 blocks
  for ( int i = 0; i <= 7; ++i )
  {
     if ( IP.section(":", i, i).isEmpty() )
       continue;

     // Check if valid hex
     IP.section(":", i,i).toInt(&ok, 16);
     if (!ok)
        return false;
  }

  return true;
}

bool Utils::validateIPv4( QString IP )
{
  int num;
  bool ok;

  if ( IP.indexOf(".") == -1 )
     return false;

  // Make sure this is a number after the . is taken out
  QString tmp;
  tmp = IP;
  tmp.replace(".", "");
  tmp.toLong(&ok);
  if (!ok)
     return false;
    
  if ( IP.count(".") != 3)
     return false;

  // Validate the V4 blocks
  for ( int i = 0; i <= 3; ++i )
  {
     if ( IP.section(".", i, i).isEmpty() )
        return false;

     if ( IP.section(".", i, i).size() > 3 )
        return false;

     num = IP.section(".", i,i).toInt(&ok);
     if (!ok)
        return false;

     if ( num > 255 || num < 0)
        return false;
  }
    
  return true;
}
