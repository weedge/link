
/*===========================================
 NAME	DATE			CONMENTS
 wy		02/2011			created

 =================================================
 *
 */
#include "LinkInfo.hpp"
#include <algorithm>

link::api::LinkInfo::LinkInfo():link::api::Link()
{

}
link::api::LinkInfo::LinkInfo(const lemur::api::TERMID_T leftid, 
							  const lemur::api::TERMID_T rightid):link::api::Link(leftid, rightid)
{

}

link::api::LinkInfo::~LinkInfo()
{

}



/**
* 获取link在窗口值为无穷大时出现的次数 
*/
link::api::LINKCOUNT_T link::api::LinkInfo::getLinkCount() const
{
	return distances.size();
}

/**
* 获取link在指定窗口大小下出现的次数 
*/
link::api::LINKCOUNT_T link::api::LinkInfo::getLinkCount(const int &windowSize) const
{
	if(distances.back() <= windowSize)//link的距离都在窗口内
	{
		return distances.size();
	}
	else
	{
		std::vector<link::api::DIST_T>::const_iterator upper = 
			upper_bound(distances.begin(), distances.end(), windowSize);//获取距离大于窗口值的第一个link的位置
		int num = 0;
		for(std::vector<link::api::DIST_T>::const_iterator it = distances.begin(); it != upper; ++it)
		{
			++num;
		}

		return num;
	}

}