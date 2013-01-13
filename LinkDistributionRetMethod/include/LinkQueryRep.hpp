#ifndef _LINK_QUERY_REP_HPP
#define _LINK_QUERY_REP_HPP
#include "Index.hpp"
#include "RetrievalMethod.hpp"
#include "LinkQuery.hpp"
#include "LinkInfoList.hpp"
#include <vector>

/**
 * ����link�ĳ��ִ������������ϸ��Ϣ
 */
class LinkQueryRep : public lemur::api::QueryRep
{
public:
	LinkQueryRep(const LinkQuery &qry, const lemur::api::Index &dbIndex);

	/** �ͷ��ڴ� */
	virtual ~LinkQueryRep();

	/** ����link����ϸ��Ϣ */
	LinkInfoList *initLinkInfo(const int &left, const int &right);
	virtual void startIteration() const {iter = 0;}
	bool hasMore() const {return iter < linkInfoListVec.size();}
	
	/** ��ȡָ��LinkInfoList�����ָ�� */
	virtual LinkInfoList *nextEntry() const;

private:
	std::vector<LinkInfoList *> linkInfoListVec;
	mutable int iter;
	const lemur::api::Index &ind;
};

#endif
