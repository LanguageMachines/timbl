/*
  Copyright (c) 1998 - 2008
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

#ifndef METRICS_H
#define METRICS_H

namespace Timbl{
  
  class metricType {
  public:
    virtual ~metricType(){};
    virtual bool isNumeric() = 0;
    virtual bool isSimilarity() = 0;
    virtual bool isStorable() = 0;
    static metricType *create( const std::string& );
  };

  class defaultMetric: public metricType {
    bool isNumeric() { return false; }
    bool isSimilarity() { return false; }
    bool isStorable() { return false; }
  };

  class ignoreMetric: public metricType {
    bool isNumeric() { return false; }
    bool isSimilarity() { return false; }
    bool isStorable() { return false; }
  };

  class numericMetric: public metricType {
    bool isNumeric() { return true; }
    bool isSimilarity() { return false; }
    bool isStorable() { return false; }
  };

  class dotproductMetric: public metricType {
    bool isNumeric() { return true; }
    bool isSimilarity() { return true; }
    bool isStorable() { return false; }
  };

  class cosineMetric: public metricType {
    bool isNumeric() { return true; }
    bool isSimilarity() { return true; }
    bool isStorable() { return false; }
  };

  class overLapMetric: public metricType {
    bool isNumeric() { return false; }
    bool isSimilarity() { return false; }
    bool isStorable() { return false; }
  };

  class levenshteinMetric: public metricType {
    bool isNumeric() { return false; }
    bool isSimilarity() { return false; }
    bool isStorable() { return true; }
  };

  class valueDiffMetric: public metricType {
    bool isNumeric() { return false; }
    bool isSimilarity() { return false; }
    bool isStorable() { return true; }
  };

  class jeffreyMetric: public metricType {
    bool isNumeric() { return false; }
    bool isSimilarity() { return false; }
    bool isStorable() { return true; }
  };

}
#endif // METRICS_H
