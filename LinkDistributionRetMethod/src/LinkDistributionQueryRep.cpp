#include "LinkDistributionQueryRep.hpp"
#include "Link.hpp"
#include "LinkInfo.hpp"

using namespace lemur::api;

LinkDistributionQueryRep::LinkDistributionQueryRep(const LinkQuery &qry, const Index &dbIndex) : LinkQueryRep(qry, dbIndex), ind(dbIndex)
{
	
}//end LinkDistributionQueryRep

