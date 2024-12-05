/*
  Copyright (c) 1998 - 2024
  ILK   - Tilburg University
  CLST  - Radboud University
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
      https://github.com/LanguageMachines/timbl/issues
  or send mail to:
      lamasoftware (at ) science.ru.nl

*/

#ifndef TIMBL_MATRICES_H
#define TIMBL_MATRICES_H

template <class T>  class SparseSymetricMatrix;
template <class T> std::ostream& operator << (std::ostream&,
					      const SparseSymetricMatrix<T>& );

template <class Class>
class SparseSymetricMatrix {
  using CDmap = std::map< Class, double >;
  using CCDmap = std::map< Class, CDmap >;
  friend std::ostream& operator << <> ( std::ostream&,
					const SparseSymetricMatrix<Class>& );

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
  };
  SparseSymetricMatrix<Class> *copy(void) const{
    SparseSymetricMatrix<Class> *res = new SparseSymetricMatrix<Class>();
    typename CCDmap::const_iterator it1 = my_mat.begin();
    while ( it1 != my_mat.end() ){
      typename CDmap::const_iterator it2 = it1->second.begin();
      while ( it2 != it1->second.end() ){
	res->my_mat[it1->first][it2->first] = it2->second;
	++it2;
      }
      ++it1;
    }
    return res;
  }
 private:
  CCDmap my_mat;
};

template <class T>
inline std::ostream& operator << (std::ostream& os,
				  const SparseSymetricMatrix<T>& m ){
  typename SparseSymetricMatrix<T>::CCDmap::const_iterator it1 = m.my_mat.begin();
  while ( it1 != m.my_mat.end() ){
    typename SparseSymetricMatrix<T>::CDmap::const_iterator it2 = it1->second.begin();
    while ( it2 != it1->second.end() ){
      os << "[" << it1->first << ",\t" << it2->first << "] "
	<< it2->second << std::endl;
      ++it2;
    }
    ++it1;
  }
  return os;
}

#endif // TIMBL_MATRICES_H
