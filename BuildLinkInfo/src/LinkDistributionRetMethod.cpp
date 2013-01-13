/**======================================
/*
 * NAME DATE - COMMENTS
 * wy  02/2012 - created to use scoreDoc(const lemur::api::Query &q, lemur::api::DOCID_T docID) function
 *========================================================================*/
#include "LinkDistributionRetMethod.hpp"
#include "Param.hpp"
#include "ResultFile.hpp"
#include "IndexedReal.hpp"
#include "indri/IndriTimer.hpp"
#include <assert.h>

using namespace lemur::api;
using namespace link::retrieval;

LinkDistributionRetMethod::LinkDistributionRetMethod(const lemur::api::Index &dbindex, lemur::api::ScoreAccumulator &accumulator, 
													 link::api::KeyfileLinkIndex *linkdbIndex):RetrievalMethod(dbindex),scAcc(accumulator),linkIndex(linkdbIndex)
{

}

LinkDistributionRetMethod::~LinkDistributionRetMethod()
{

}//end ~LinkDistributionRetMethod

QueryRep *LinkDistributionRetMethod::computeQueryRep(const Query &qry)
{
	//if (const LinkQuery *q = dynamic_cast<const LinkQuery *>(&qry))
	//	return (new LinkDistributionQueryRep(*q, ind));
 //   else 
 //       LEMUR_THROW(LEMUR_RUNTIME_ERROR, "LinkDistributionRetMethod expects a LinkQuery object");
	return NULL;
}//end computeQueryRep

/*
DocumentRep *LinkDistributionRetMethod::computeDocRep(DOCID_T docID)
{
	return (new LinkDistributionDocRep(docID, ind));
}//end computeDocRep
*/

void LinkDistributionRetMethod::scoreCollection(const QueryRep &qry, IndexedRealVector &results)
{

}//end scoreCollection


void LinkDistributionRetMethod::scoreCollection(DOCID_T docid, IndexedRealVector &results)
{

}//end scoreCollection

/** 
 * 计算查询与文档的得分。
 * 优化：可以通过容器来保存已经计算过的模型中所需要的数据,而不需要重新去磁盘文件中找。
 */
double LinkDistributionRetMethod::scoreDoc(const lemur::api::Query &q, lemur::api::DOCID_T docID)
{
	//assert(LinkDistributionParameter::docWindowSize>0);
	double score = 0.0;
	int docCount = ind.docCount();
	int colLength = ind.termCount();
	int winSize = lemur::api::ParamGetInt("docWindowSize", LinkDistributionParameter::defaultDocWindowSize);
	int docLength = ind.docLength(docID);

	const link::retrieval::LinkQuery* lq = dynamic_cast<const LinkQuery *> (&q);
	lq->startLinkIteration();

	while (lq->hasMore())
	{
		int i;
		int linkCountDocX = 0;//link在某窗口下，某文档中出现的次数;如果没有则初始为0。
		link::api::LINKCOUNT_T linkCountCol = 0;//link在集合中出现的次数。如果没有则初始为0。
		link::api::LINKCOUNT_T linkCountColX = 0;//link在某窗口下,集合中出现的次数。如果没有则初始为0。
		link::api::Link *li = const_cast<link::api::Link *> (lq->nextLink());
		//cerr<<"link:"<<li->toString()<<endl;
		link::api::LINKID_T linkID = linkIndex->linkTerm(li->toString());	
		const lemur::api::LOC_T *linkCountWins = linkIndex->linkCountX(linkID,docID);//link在指定文档、某些指定窗口大小时出现的次数
		vector<int> winSizes = linkIndex->winSizes();
		if (linkCountWins!=NULL)
		{
			for (i=0;i<linkIndex->winCount(); i++)
			{
				if (winSizes[i]==winSize)
				{
					linkCountDocX = linkCountWins[i];//link在某窗口下，文档中出现的次数，窗口为winSize
					break;
				}
				else
				{
					continue;
				}
			}
			if(i==(linkIndex->winCount()))
			{
				cerr<<"LinkDistributionParameter::docWindowSize is not in the linkIndex,so it is invalid!";
				exit(-1);
			}
			delete linkCountWins;
			linkCountWins = NULL;
		}


		double tmp1 = (winSize-1) * (docLength - (winSize/2.0));
		double probD = linkCountDocX/tmp1;
		//double probD = 1.0 * linkCountDocX/docLength;

		
		if(probD < 0)
		{
			cerr << "docLength : " << docLength << endl;
			cerr << probD << "\tprobD < 0" << endl;
			//system("pause");
			//exit(1);
		}
		

		if(probD < 0)
			probD = abs(probD);


		link::api::LINKCOUNT_T *linkCountColWins = linkIndex->linkCountX(linkID);//link在文档集中、某些指定窗口大小时出现的次数
		if (linkCountColWins!=NULL)
		{
			for (i = 0;i<linkIndex->winCount(); i++)
			{
				if (winSizes[i]==winSize)
				{
					linkCountColX = linkCountColWins[i];//link在某窗口下集合中出现的次数，窗口为winSize
					break;
				}
				else
				{
					continue;
				}
			}
			if(i==(linkIndex->winCount()))
			{
				cerr<<"LinkDistributionParameter::docWindowSize is not in the linkIndex,so it is invalid!";
				exit(-1);
			}
			delete linkCountColWins;
			linkCountColWins = NULL;
		}

		linkCountCol = linkIndex->linkCount(linkID);//link在集合中出现的次数，窗口为无穷大
		double alpha = lemur::api::ParamGetInt("useLinkDist", 1) ? 1.0/linkCountCol : 1.0;
		
		double tmp2 = 1.0*(winSize - 1)*(colLength - docCount*winSize/2);
		double probC = (alpha * linkCountColX)/tmp2;
		//double probC = 1.0 * (alpha * linkCountColX)/colLength;

		
		if(probC < 0)
		{
			cerr << "colLength : " << colLength << endl;
			cerr << "docCount*WindowSize/2 : " << docCount*winSize/2 << endl; 
			cerr << "colLength - docCount*WindowSize/2 : " << colLength - docCount*winSize/2 << endl;
			cerr << probC << "\tprobC < 0" << endl;
			//system("pause");
			//exit(1);
		}
		

		if(probC < 0)
			probC = abs(probC);


		double eta = lemur::api::ParamGetDouble("eta", 0.75);
	
		double prob = (1-eta) * probD + eta * probC;
		if(prob == 0) continue;//某些link在集合中出现的次数很少，例如2次，在指定窗口为40时，出现次数为0，则prob=0
		score += log(prob);

		switch(lemur::api::ParamGetInt("debug", 0))
		{
		case 1:
			std::cout << ". " ;
			break;
		case 2:
			li->linkToPair((li->toString()));
			std::cout << "==============================================================================" << std::endl;
			std::cout << li->getLeftID() << " " <<ind.term(li->getLeftID()) << " " <<li->getRightID() << ind.term(li->getRightID()) <<endl;
			std::cout << "linkCountDoc : " << linkCountDocX << "\tdocLength : " << docLength << "\tWindowSize : " << winSize << std::endl;
			std::cout << "p(l|d) : " << probD << std::endl;
			std::cout << "linkCountColX : " << linkCountColX << "\tlinkCountCol : " << linkCountCol << "\talpha : " << alpha;
			std::cout << "\tdocCount : " << docCount << "\tcolLength : " << colLength << endl;
			std::cout << "p(l|c) : " << probC << std::endl;
			std::cout << "(1-eta) * probD + eta * probC : " << prob << std::endl;
			std::cout << "score(log) : " << score << std::endl;
			std::cout << "==============================================================================" << std::endl;	
		default:
			break;
		}
	}//while

	if(lemur::api::ParamGetInt("debug", 0))
	{
		std::cout << std::endl << "score : " << score << std::endl;
	}

	return score;

}



double LinkDistributionRetMethod::scoreDoc(const lemur::api::QueryRep &qry, lemur::api::DOCID_T docID)
{
	double score = 0.0;
	return score;
}//end scoreDoc

/** Score a set of documents w.r.t. a query rep (e.g. for re-ranking) */
void LinkDistributionRetMethod::scoreDocSet(const QueryRep &qry, const DocIDSet &docSet, IndexedRealVector &results)
{
	results.clear();
	docSet.startIteration();

	while(docSet.hasMore())
	{
		int docID;
		double prevScore;
		docSet.nextIDInfo(docID, prevScore);
		
		double phi = lemur::api::ParamGetDouble("phi", 0);
		double currentScore = (1 - phi) * prevScore + phi * scoreDoc(qry, docID);
		results.PushValue(docID, currentScore);
	}//while
}//end scoreDocSet

void LinkDistributionRetMethod::scoreDocSet(const Query &q, const DocIDSet &docSet, IndexedRealVector &results)
{
	//assert(LinkDistributionParameter::docWindowSize>0);

	results.clear();
	docSet.startIteration();
	while(docSet.hasMore())
	{
		int docID;
		double prevScore;
		docSet.nextIDInfo(docID, prevScore);
		//cerr<<"docID:"<<docID<<endl;
		double phi = lemur::api::ParamGetDouble("phi", 0);
		double currentScore = (1 - phi) * prevScore + phi * scoreDoc(q, docID);
		results.PushValue(docID, currentScore);
	}//while

}

void LinkDistributionRetMethod::updateQuery(QueryRep &qryRep, const lemur::api::DocIDSet &relDocs)
{

}//end updateQuery