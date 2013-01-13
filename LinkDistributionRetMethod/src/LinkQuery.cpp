#include "LinkQuery.hpp"
#include "LinkDistributionRetMethod.hpp"
#include <vector>
#include <algorithm>

using namespace lemur::api;

/**
 * 构造函数，将query文档中的term生成link
 */
LinkQuery::LinkQuery(const Document &doc, const Index &dbindex) : iter(0)
{
	if(doc.getID())
	{
		qid = doc.getID();

		vector<int> termIDVec;

		doc.startTermIteration();
		while(doc.hasMore())
		{
			int termID = dbindex.term(doc.nextTerm()->spelling());
			if(termID != 0)
			{
				termIDVec.push_back(termID);
			}
			
		}

		sort(termIDVec.begin(), termIDVec.end());
		vector<int>::iterator endUnique = unique(termIDVec.begin(), termIDVec.end());

		termIDVec.erase(endUnique, termIDVec.end());

		if(lemur::api::ParamGetInt("debug", 0))
		{
			std::cout << "Terms in query " << doc.getID() << ": " << endl;
			for(vector<int>::iterator it = termIDVec.begin(); it != termIDVec.end(); ++it)
			{
				std::cout << dbindex.term(*it) << "\t" << *it << endl;
			}//for
			
			std::cout << "Total unique terms in query " << doc.getID() << " : " << termIDVec.size() << endl;
		}//if

		for(int i = 0; i != termIDVec.size() -1; ++i)
		{
			for(int j = i+1; j != termIDVec.size(); ++j)
			{
				pair<int, int> lnk(termIDVec[i], termIDVec[j]);
				linkList.push_back(lnk);
			}
		}//for

		if(lemur::api::ParamGetInt("debug", 0))
		{
			int i = 1;
			for(vector<pair<int, int> >::iterator it = linkList.begin(); it != linkList.end(); ++it, ++i)
			{
				std::cout << i << "--> < " << (*it).first << "," << (*it).second << " >" << std::endl;
			}//for

			std::cout << "Total unique links in query " << doc.getID() << " : " << linkList.size() << endl;
		}//if

	}//if
	else qid = "";

}//end LinkQuery

Link *LinkQuery::nextLink() const
{
	link.setLeftID(linkList[iter].first);
	link.setRightID(linkList[iter].second);
	++iter;
	return &link;
}//end nextLink
