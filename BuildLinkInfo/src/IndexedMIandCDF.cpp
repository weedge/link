#include "IndexedMIandCDF.hpp"
#include <cmath>
#include <cassert>
using namespace link::api;

IndexedMIandCDFVector::IndexedRealMIAscending IndexedMIandCDFVector::ascendOrderMI;
IndexedMIandCDFVector::IndexedRealMIDescending IndexedMIandCDFVector::descendOrderMI;
IndexedMIandCDFVector::IndexedRealCDFAscending IndexedMIandCDFVector::ascendOrderCDF;
IndexedMIandCDFVector::IndexedRealCDFDescending IndexedMIandCDFVector::descendOrderCDF;

void link::api::IndexedMIandCDFVector::Sort(int opt /*= MI */,bool descending /* = true */)
{
	if (opt == MI)
	{
		if (descending)
		{
			stable_sort(begin(),end(),descendOrderMI);
		}
		else
		{
			stable_sort(begin(),end(),ascendOrderMI);
		}
		
	}
	if (opt == CDF)
	{
		if (descending)
		{
			stable_sort(begin(),end(),descendOrderCDF);
		}
		else
		{
			stable_sort(begin(),end(),ascendOrderCDF);
		}
	}
}

void IndexedMIandCDFVector::PushValue(int index, double valueMI, double valueCDF)
{
	IndexedMIandCDF enty;
	enty.ind = index;
	enty.valMI = valueMI;
	enty.valCDF = valueCDF;
	push_back(enty);
}

//在一个区间范围内的值相加得到平均值
double IndexedMIandCDFVector::getAvgValue(double from, double to, int &count,int opt /*= MI */)
{
	//assert(from!=to);
	if (from>to)
	{
		return 0.0;
	}
	double sum = 0.0;
	count = 0;
	iterator iter = begin();
	if (opt==MI)
	{
		for (;iter!=end();iter++)
		{
			if( (*iter).valMI>from && (*iter).valMI<=to )
			{
				sum += (*iter).valMI;
				count++;
			}
		}
		if (sum!=0.0)
		{
			return sum/count;
		}
	}
	else if (opt==CDF)
	{
		for (;iter!=end();iter++)
		{
			if( (*iter).valCDF>from && (*iter).valCDF<=to )
			{
				sum += (*iter).valCDF;
				count++;
			}
		}
		if (sum!=0.0)
		{
			return sum/count;
		}
	}
	return 0.0;
}

double IndexedMIandCDFVector::getAvgValueByReal(double from, double to, int &count, int opt /*= MI */)
{
	//assert(from!=to);
	if (from>to)
	{
		return 0.0;
	}
	double sum = 0.0;
	count = 0;
	iterator iter = begin();
	if (opt==MI)
	{
		for (;iter!=end();iter++)
		{
			if( (*iter).valMI>from && (*iter).valMI<=to )
			{
				sum += (*iter).valCDF;
				count++;
			}
		}
		if (sum!=0.0)
		{
			return sum/count;
		}
	}
	else if (opt==CDF)
	{
		for (;iter!=end();iter++)
		{
			if( (*iter).valCDF>from && (*iter).valCDF<=to )
			{
				sum += (*iter).valMI;
				count++;
			}
		}
		if (sum!=0.0)
		{
			return sum/count;
		}
	}
	return 0.0;
}

double IndexedMIandCDFVector::getMaxValue(int opt /* = MI */)
{
	double MAX = 0.0;
	iterator iter = begin();
	if (opt==CDF)
	{
		for (;iter!=end();iter++)
		{
			if( (*iter).valCDF> MAX )
			{
				MAX = (*iter).valCDF;
			}
		}
	}
	if (opt==MI)
	{
		for (;iter!=end();iter++)
		{
			if( (*iter).valMI> MAX)
			{
				MAX = (*iter).valMI;
			}
		}

	}
	return MAX;

}