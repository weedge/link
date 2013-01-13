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
 * 当窗口为无穷大时，link在集合中出现的次数
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

/** 获取link在集合中出现的次数，窗口为指定大小 */
int LinkInfoList::getCount(const int &windowSize) const
{
	int count = 0;
	for(std::vector<LinkInfo *>::const_iterator it = linkInfoVec.begin(); it != linkInfoVec.end(); ++it)
	{
		count += (*it)->getCount(windowSize);
	}//for

	return count;
}//end getCount

/** 获取link在指定文档、指定窗口大小时出现的次数 */
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
 * 析构函数，释放内存
 */
LinkInfoList::~LinkInfoList()
{
	if(lemur::api::ParamGetInt("debug", 0))
	{
		std::cout << "~LinkInfoList 析构函数被调用，释放 LinkInfo 对象所占内存" << endl;
	}

	while(!linkInfoVec.empty())
	{
		delete linkInfoVec.back();
		linkInfoVec.back() = 0;
		linkInfoVec.pop_back();
	}//while
}//end ~LinkInfo
