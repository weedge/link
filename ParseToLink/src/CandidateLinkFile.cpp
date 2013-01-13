
/**======================================
/*
 * NAME DATE - COMMENTS
 * wy  05/2011 - created
 *========================================================================*/

#include "CandidateLinkFile.hpp"
#include <cassert>

link::file::CandidateLinkFile::CandidateLinkFile(bool LINKFormat)
{
	this->linkFmt = LINKFormat;
}

bool link::file::CandidateLinkFile::findLink(const std::string &exceptedLink)
{
	std::set<link::api::LINK_STR>::const_iterator iter;
	iter = this->PairIDs.find(exceptedLink);
	if(iter != PairIDs.end())
	{
		return true;
	}

	return false;
}

bool link::file::CandidateLinkFile::readLine()
{
	char dummy1[100];//term-term

	if(!inStr->eof())
	{
		if (linkFmt){

			return (*inStr >> curPairID >> dummy1);
		}else{

			return (*inStr >> curPairID);
		}
	}else{
		return false;
	}

}

void link::file::CandidateLinkFile::readToVector()
{
	char dummy1[100];//term-term

	while(!inStr->eof())
	{
		if(linkFmt)
		{
			*inStr >> curPairID >> dummy1;
		}else{
			*inStr >> curPairID;
		}
		PairIDs.insert(curPairID);
	}
}

void link::file::CandidateLinkFile::writeResults(const link::api::Link &li)
{
	lemur::api::TERMID_T leftTID = li.getLeftID();
	lemur::api::TERMID_T rightTID = li.getRightID();
	
	if(linkFmt)
	{
		*outStr<<leftTID<<"-"<<rightTID<<" "<<ind->term(leftTID)<<"-"<<ind->term(rightTID)<<std::endl;
	}else
	{
		*outStr<<leftTID<<"-"<<rightTID<<std::endl;
	}
}
