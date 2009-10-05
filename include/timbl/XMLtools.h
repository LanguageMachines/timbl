#ifndef XML_TOOLS_H
#define XML_TOOLS_H

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
