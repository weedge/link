#ifndef _LINK_INFO_LIST_HPP
#define _LINK_INFO_LIST_HPP

#include <vector>
#include "LinkInfo.hpp"

/**
 * 描述分布在不同文档中的同一个link
 */
class LinkInfoList
{
public:
	LinkInfoList(){}
	
	/** 释放内存 */
	virtual ~LinkInfoList();

	void addLinkInfo(LinkInfo *li);

	virtual void startIteration() const {iter = 0;}

	bool hasMore() const {return iter < linkInfoVec.size();}

	virtual LinkInfo *nextEntry() const;

	/** 当窗口为无穷大时，link在集合中出现的次数 */
	virtual int getCount() const;

	/** 获取link在集合中出现的次数，窗口为指定大小 */
	virtual int getCount(const int &windowSize) const;

	/** 获取link在指定文档、指定窗口大小时出现的次数 */
	virtual int getCount(const int &docID, const int &windowSize) const;

protected:
	std::vector<LinkInfo *> linkInfoVec;
	mutable int iter;
};

#endif
