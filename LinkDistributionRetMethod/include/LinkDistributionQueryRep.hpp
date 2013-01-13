#ifndef _LINK_DISTRIBUTION_QUERY_REP_HPP
#define _LINK_DISTRIBUTION_QUERY_REP_HPP

#include "Index.hpp"
#include "LinkQuery.hpp"
#include "LinkQueryRep.hpp"
#include <vector>

class LinkDistributionQueryRep : public LinkQueryRep
{
public:
	LinkDistributionQueryRep(const LinkQuery &qry, const lemur::api::Index &dbIndex);
	virtual ~LinkDistributionQueryRep(){}


private:
	const lemur::api::Index &ind;
};

#endif
