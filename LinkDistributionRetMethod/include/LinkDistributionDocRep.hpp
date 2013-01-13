#ifndef _LINK_DISTRIBUTION_DOC_REP_HPP
#define _LINK_DISTRIBUTION_DOC_REP_HPP

#include "DocumentRep.hpp"
#include "Index.hpp"

class LinkDistributionDocRep : public lemur::api::DocumentRep
{
public:
	LinkDistributionDocRep(lemur::api::DOCID_T docID, const lemur::api::Index &dbIndex) : lemur::api::DocumentRep(docID, dbIndex.docLength(docID)), ind(dbIndex) {}
	virtual ~LinkDistributionDocRep() {}

	virtual double scoreConstant() const { return 0;}
	virtual double termWeight(lemur::api::TERMID_T termID, const lemur::api::DocInfo *info) const{return 0;}
private:
	const lemur::api::Index &ind;
};

#endif
