/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2013
  ILK   - Tilburg University
  CLiPS - University of Antwerp
 
  This file is part of timbl

  timbl is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  timbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      timbl@uvt.nl
*/

#ifndef TIMBL_XML_TOOLS_H
#define TIMBL_XML_TOOLS_H

#include <list>
#include "libxml/tree.h"

xmlNode *XmlNewNode( const std::string& );
xmlNode *XmlNewComment( const std::string& );
xmlNode *XmlNewChild( xmlNode *, 
		      const std::string& , const std::string& ="" );
xmlNode *XmlAddChild( xmlNode *, xmlNode * );
void XmlAddContent( xmlNode *, const std::string& );

bool XmlSetAttribute( xmlNode *, const std::string&, const std::string& );

std::string serialize( const xmlNode& node );

class XmlDoc {
  friend std::ostream& operator << ( std::ostream& , const XmlDoc& );
 public:
  XmlDoc( const std::string& );
  ~XmlDoc( );
  void setRoot( xmlNode* );
  xmlNode *getRoot() const;
  xmlNode *MakeRoot( const std::string& );
  const std::string toString() const;
 private:
  xmlDoc *the_doc;
};

inline std::ostream& operator << ( std::ostream& os, const XmlDoc& doc ){
  os << doc.toString();
  return os;
}

inline std::ostream& operator << ( std::ostream& os, const xmlNode& node ){
  os << serialize( node );
  return os;
}

inline std::ostream& operator << ( std::ostream& os, const xmlNode *node ){
  os << serialize( *node );
  return os;
}

#endif
