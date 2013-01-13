
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
* ��ȡlink�ڴ���ֵΪ�����ʱ���ֵĴ��� 
*/
link::api::LINKCOUNT_T link::api::LinkInfo::getLinkCount() const
{
	return distances.size();
}

/**
* ��ȡlink��ָ�����ڴ�С�³��ֵĴ��� 
*/
link::api::LINKCOUNT_T link::api::LinkInfo::getLinkCount(const int &windowSize) const
{
	if(distances.back() <= windowSize)//link�ľ��붼�ڴ�����
	{
		return distances.size();
	}
	else
	{
		std::vector<link::api::DIST_T>::const_iterator upper = 
			upper_bound(distances.begin(), distances.end(), windowSize);//��ȡ������ڴ���ֵ�ĵ�һ��link��λ��
		int num = 0;
		for(std::vector<link::api::DIST_T>::const_iterator it = distances.begin(); it != upper; ++it)
		{
			++num;
		}

		return num;
	}

}