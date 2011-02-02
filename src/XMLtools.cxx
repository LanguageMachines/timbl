/*
  $Id$
  $URL$

  Copyright (c) 1998 - 2011
  ILK  -  Tilburg University
  CNTS -  University of Antwerp
 
  This file is part of Timbl

  Timbl is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Timbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.

  For questions and suggestions, see:
      http://ilk.uvt.nl/software.html
  or send mail to:
      Timbl@uvt.nl
*/

#include <iostream>
#include <string>
#include "timbl/XMLtools.h"

using namespace std;

XmlDoc::XmlDoc( const std::string& elem ){
  the_doc = xmlNewDoc( (const xmlChar*)"""1.0""" );
  MakeRoot( elem );
}

XmlDoc::~XmlDoc(){
  xmlFreeDoc( the_doc );
}

const string XmlDoc::toString() const {
  xmlChar *buf; 
  int size;
  xmlDocDumpMemory( the_doc, &buf, &size );
  const string result = string( (const char *)buf, size );
  xmlFree( buf );
  return result;
}

xmlNode *XmlDoc::getRoot() const {
  if ( the_doc )
    return xmlDocGetRootElement(the_doc);
  else
    return 0;
}

void XmlDoc::setRoot( xmlNode *node ){
  if ( the_doc )
    xmlDocSetRootElement(the_doc, node );
}

xmlNode *XmlDoc::MakeRoot( const string& elem ){
  xmlNode *root;
  root = xmlNewDocNode( the_doc, 0, (const xmlChar*)elem.c_str(), 0 );
  xmlDocSetRootElement( the_doc, root );
  return root;
}

xmlNode *XmlNewNode( const string& elem ){
  return xmlNewNode( 0, (const xmlChar*)elem.c_str() );
}

xmlNode *XmlNewComment( const string& elem ){
  return xmlNewComment( (const xmlChar*)elem.c_str() );
}

xmlNode *XmlNewChild( xmlNode *node, const string& elem, const string& val ){
  if ( val.empty() )
    return xmlNewTextChild( node, 0, (xmlChar*)elem.c_str(), 0 );
  else 
    return xmlNewTextChild( node, 0, 
			    (const xmlChar*)elem.c_str(),
			    (const xmlChar*)val.c_str() );
}

xmlNode *XmlAddChild( xmlNode *node, xmlNode *elem ){
  return xmlAddChild( node, elem );
}

void XmlAddContent( xmlNode *node, const string& cont ){
  xmlNodeAddContent( node, (const xmlChar*)cont.c_str() );
}

bool XmlSetAttribute( xmlNode *node, 
		      const std::string& att,
		      const std::string& val ){
  if ( node ){
    return xmlSetProp( node, 
		       (const xmlChar*)att.c_str(), 
		       (const xmlChar*)val.c_str() )!= 0;
  }
  else
    return false;
}

string serialize( const xmlNode& node ){
  XmlDoc tmp( "root" );
  xmlAddChild( tmp.getRoot(), xmlCopyNode( const_cast<xmlNode*>(&node), 1 ) );
  string root = tmp.toString();
  string::size_type s_pos = root.find( "<root>" );
  string::size_type e_pos = root.find( "</root>" );
  return root.substr( s_pos+6, e_pos-s_pos-6 );
}
