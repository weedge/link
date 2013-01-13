#ifndef _LINK_INFO_LIST_HPP
#define _LINK_INFO_LIST_HPP

#include <vector>
#include "LinkInfo.hpp"

/**
 * �����ֲ��ڲ�ͬ�ĵ��е�ͬһ��link
 */
class LinkInfoList
{
public:
	LinkInfoList(){}
	
	/** �ͷ��ڴ� */
	virtual ~LinkInfoList();

	void addLinkInfo(LinkInfo *li);

	virtual void startIteration() const {iter = 0;}

	bool hasMore() const {return iter < linkInfoVec.size();}

	virtual LinkInfo *nextEntry() const;

	/** ������Ϊ�����ʱ��link�ڼ����г��ֵĴ��� */
	virtual int getCount() const;

	/** ��ȡlink�ڼ����г��ֵĴ���������Ϊָ����С */
	virtual int getCount(const int &windowSize) const;

	/** ��ȡlink��ָ���ĵ���ָ�����ڴ�Сʱ���ֵĴ��� */
	virtual int getCount(const int &docID, const int &windowSize) const;

protected:
	std::vector<LinkInfo *> linkInfoVec;
	mutable int iter;
};

#endif
