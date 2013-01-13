
#ifndef _INDEXEDMIANDCDF_HPP
#define _INDEXEDMIANDCDF_HPP

#include "common_headers.hpp"
#include <algorithm>

#define MI 0
#define CDF 1

namespace link 
{
  namespace api 
  {
    
    /// A list of indexed real numbers (similar to IndexProb)

    // CLASSES: IndexedMIandCDF, IndexedMIandCDFVector (wy, 12/02/2012) U can see the IndexedReal.hpp

    struct IndexedMIandCDF {
      int ind; // index
      double valMI;
	  double valCDF;
    };

    /// \brief A vector of IndexedMIandCDF.
    /// 
    class IndexedMIandCDFVector : public vector<IndexedMIandCDF> {
    public:
  
      IndexedMIandCDFVector() : vector<IndexedMIandCDF>() {}
      IndexedMIandCDFVector(int size) : vector<IndexedMIandCDF>(size) {}
      virtual ~IndexedMIandCDFVector() {}
      //virtual iterator FindByIndex(int index);

      /// increase the value for "index", add the entry if not existing.
      // return true iff the entry already exists.
     // virtual bool IncreaseValueFor(int index, double valueMI, double valueCDF);

      /// push a value 
      virtual void PushValue(int index, double valueMI, double valueCDF);

      /// sort all the values,  default is descending order using MI
      virtual void Sort(int opt = MI,bool descending = true);

      /// get the avg value from a to b. count can get ~!
      virtual double getAvgValue(double from, double to, int &count, int opt = MI);
	  /// get the avg value from a to b  through the RealValue ex:MI[] -> avgCDF. and count can get ~!
	  virtual double getAvgValueByReal(double from, double to, int &count, int opt = MI);
	
	  /// get the max value 
	  virtual double getMaxValue(int opt = MI);

      /// consider input values as log(x), mapping to exp(log(x)), and
      /// normalize x values in a range (0:1), and sum to 1
      //virtual void LogToPosterior();


    private:

      /// Function object types, 
      /// defines how to compare IndexedMIorCDF objects (for sorting/searching)

      class IndexedRealMIAscending {
      public:
        bool operator()(const IndexedMIandCDF & a, const IndexedMIandCDF & b) {
          return a.valMI < b.valMI;  // based on the real value
        }
      };

      class IndexedRealMIDescending { 
      public: 
        bool operator()(const IndexedMIandCDF & a, const IndexedMIandCDF & b) {
          return a.valMI > b.valMI;  // based on the real value
        }
      };

	  class IndexedRealCDFAscending {
	  public:
		  bool operator()(const IndexedMIandCDF & a, const IndexedMIandCDF & b) {
			  return a.valCDF < b.valCDF;  // based on the real value
		  }
	  };

	  class IndexedRealCDFDescending { 
	  public: 
		  bool operator()(const IndexedMIandCDF & a, const IndexedMIandCDF & b) {
			  return a.valCDF > b.valCDF;  // based on the real value
		  }
	  };
	  static IndexedRealMIAscending ascendOrderMI;
	  static IndexedRealMIDescending descendOrderMI;

	  static IndexedRealCDFAscending ascendOrderCDF;
	  static IndexedRealCDFDescending descendOrderCDF;

    };
  }
}
#endif //_INDEXEDREAL_HPP
