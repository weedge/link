#ifndef _LINK_QUERY_REP_HPP
#define _LINK_QUERY_REP_HPP
#include "Index.hpp"
#include "RetrievalMethod.hpp"
#include "LinkQuery.hpp"
#include "LinkInfoList.hpp"
#include <vector>

/**
 * 生成link的出现次数，距离等详细信息
 */
class LinkQueryRep : public lemur::api::QueryRep
{
public:
	LinkQueryRep(const LinkQuery &qry, const lemur::api::Index &dbIndex);

	/** 释放内存 */
	virtual ~LinkQueryRep();

	/** 设置link的详细信息 */
	LinkInfoList *initLinkInfo(const int &left, const int &right);
	virtual void startIteration() const {iter = 0;}
	bool hasMore() const {return iter < linkInfoListVec.size();}
	
	/** 获取指向LinkInfoList对象的指针 */
	virtual LinkInfoList *nextEntry() const;

private:
	std::vector<LinkInfoList *> linkInfoListVec;
	mutable int iter;
	const lemur::api::Index &ind;
};

#endif
