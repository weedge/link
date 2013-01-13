#include "LinkInfoList.hpp"
#include "Param.hpp"

using namespace lemur::api;

void LinkInfoList::addLinkInfo(LinkInfo *li)
{
	linkInfoVec.push_back(li);
}//end addLinkInfo

LinkInfo *LinkInfoList::nextEntry() const
{
	return linkInfoVec[iter++];
}//end nextEntry

/**
 * ������Ϊ�����ʱ��link�ڼ����г��ֵĴ���
 */
int LinkInfoList::getCount() const
{
	int count = 0;
	for(std::vector<LinkInfo *>::const_iterator it = linkInfoVec.begin(); it != linkInfoVec.end(); ++it)
	{
		count += (*it)->getCount();
	}//for

	return count;
}//end getCount

/** ��ȡlink�ڼ����г��ֵĴ���������Ϊָ����С */
int LinkInfoList::getCount(const int &windowSize) const
{
	int count = 0;
	for(std::vector<LinkInfo *>::const_iterator it = linkInfoVec.begin(); it != linkInfoVec.end(); ++it)
	{
		count += (*it)->getCount(windowSize);
	}//for

	return count;
}//end getCount

/** ��ȡlink��ָ���ĵ���ָ�����ڴ�Сʱ���ֵĴ��� */
int LinkInfoList::getCount(const int &docID, const int &windowSize) const
{
	int count = 0;
	for(std::vector<LinkInfo *>::const_iterator it = linkInfoVec.begin(); it != linkInfoVec.end(); ++it)
	{
		if((*it)->getDocID() == docID)
		{
			count = (*it)->getCount(windowSize);
			return count;
		}
	}//for

	return count;
}//end getCount

/**
 * �����������ͷ��ڴ�
 */
LinkInfoList::~LinkInfoList()
{
	if(lemur::api::ParamGetInt("debug", 0))
	{
		std::cout << "~LinkInfoList �������������ã��ͷ� LinkInfo ������ռ�ڴ�" << endl;
	}

	while(!linkInfoVec.empty())
	{
		delete linkInfoVec.back();
		linkInfoVec.back() = 0;
		linkInfoVec.pop_back();
	}//while
}//end ~LinkInfo
