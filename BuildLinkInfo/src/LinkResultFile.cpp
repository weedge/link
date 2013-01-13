
#include "Link.hpp"
#include "LinkResultFile.hpp"
#include "common_headers.hpp"

void link::file::InvLinkInfoFile::writeLinkResultToFile( link::api::IndexedMIandCDFVector *results)
{

	link::api::IndexedMIandCDFVector::iterator i;
	for (i=results->begin(); i!=results->end(); i++)
	{
		std::string linkStr = linkInd->linkTerm((*i).ind);
		*outLinkfile<<linkStr<<" ";
		if (LinkFmt)
		{
			link::api::Link li;
			li.linkToPair(linkStr);
			*outLinkfile<<ind->term(li.getLeftID())<<"-"<<ind->term(li.getRightID())<<" ";
		}

		*outLinkfile<<(*i).valMI<<" "<<(*i).valCDF<<endl;
	}
}

void link::file::InvLinkInfoFile::writeLinkResultToFile(lemur::api::IndexedRealVector *results)
{
	lemur::api::IndexedRealVector::iterator i;
	for (i=results->begin(); i!=results->end(); i++)
	{
		std::string linkStr = linkInd->linkTerm((*i).ind);
		*outLinkfile<<linkStr<<" ";
		if (LinkFmt)
		{
			link::api::Link li;
			li.linkToPair(linkStr);
			*outLinkfile<<ind->term(li.getLeftID())<<"-"<<ind->term(li.getRightID())<<" ";
		}

		*outLinkfile<<(*i).val<<endl;
	}
}
