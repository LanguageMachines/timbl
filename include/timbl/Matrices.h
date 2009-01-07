/*
  Copyright (c) 1998 - 2009
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

#ifndef MATRICES_H
#define MATRICES_H

  template <class Class>
    class SparseSymetricMatrix {
    typedef std::map< Class, double > CDmap;
    typedef std::map< Class, CDmap > CCDmap;
  public:
    void Clear() { my_mat.clear(); };
    void Assign( Class i, Class j, double d ){ 
      if ( i == j )
	return;
      if ( i <j )
	my_mat[j][i] = d; 
      else
	my_mat[i][j] = d; 
    };
    double Extract( Class i, Class j ) const { 
      if ( i == j ){
	return 0.0;
      }
      if ( i < j ){
	typename CCDmap::const_iterator it1 = my_mat.find(j);
	if ( it1 != my_mat.end() ){
	  typename CDmap::const_iterator it2 = it1->second.find(i);
	  if ( it2 != it1->second.end() ){
	    return it2->second; 
	  }
	}
      }
      else {
	typename CCDmap::const_iterator it1 = my_mat.find(i);
	if ( it1 != my_mat.end() ){
	  typename CDmap::const_iterator it2 = it1->second.find(j);
	  if ( it2 != it1->second.end() ){
	    return it2->second; 
	  }
	}
      }
      return 0.0;
    };
    unsigned int NumBytes(void) const{
      unsigned int tot = sizeof(std::map<Class, CDmap>);
      typename CCDmap::const_iterator it1 = my_mat.begin();
      while ( it1 != my_mat.end() ){
	tot +=  sizeof(CDmap);
	typename CDmap::const_iterator it2 = it1->second.begin();
	while ( it2 != it1->second.end() ){
	  tot += sizeof(double);
	  ++it2;
	}
	++it1;
      }
      return tot;
    }
  private:
    CCDmap my_mat;
  };

#endif // MATRICES_H
