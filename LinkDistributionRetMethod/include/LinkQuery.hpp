#ifndef _LINK_QUERY_HPP
#define _LINK_QUERY_HPP

#include "RetrievalMethod.hpp"
#include "Document.hpp"
#include "Link.hpp"
#include <vector>
#include <utility>

class LinkQuery : public lemur::api::Query
{
public:
	LinkQuery(){}
	LinkQuery(const lemur::api::Document &doc, const lemur::api::Index &dbindex);
	virtual ~LinkQuery(){}
	bool hasMore() const{ return iter < linkList.size();}
	virtual Link *nextLink() const;
	virtual void startIteration() const{iter = 0;}

protected:
	vector<pair<int, int> > linkList;
	mutable int iter;
	mutable Link link;
};

#endif
