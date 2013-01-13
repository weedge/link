#include "LinkDistributionRetMethod.hpp"
#include "LinkQuery.hpp"
#include "LinkQueryRep.hpp"
#include "IndexManager.hpp"
#include "Param.hpp"
#include "DocStream.hpp"
#include "BasicDocStream.hpp"
#include "RetParamManager.hpp"
#include "ResultFile.hpp"
#include "IndexedReal.hpp"
#include "RetrievalMethod.hpp"
#include "TextQueryRep.hpp"
#include "LinkDistributionQueryRep.hpp"
#include "LinkDistributionDocRep.hpp"
#include "LinkQuery.hpp"
#include "indri/IndriTimer.hpp"
#include <sstream>

using namespace lemur::api;

LinkDistributionRetMethod::LinkDistributionRetMethod(const lemur::api::Index &dbindex, lemur::api::ScoreAccumulator &accumulator) : RetrievalMethod(dbindex), scAcc(accumulator)
{

}//end LinkDistributionRetMethod

LinkDistributionRetMethod::~LinkDistributionRetMethod()
{

}//end ~LinkDistributionRetMethod

QueryRep *LinkDistributionRetMethod::computeQueryRep(const Query &qry)
{
	if (const LinkQuery *q = dynamic_cast<const LinkQuery *>(&qry))
		return (new LinkDistributionQueryRep(*q, ind));
    else 
        LEMUR_THROW(LEMUR_RUNTIME_ERROR, "LinkDistributionRetMethod expects a LinkQuery object");
	
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
 * 计算文档得分
 */
double LinkDistributionRetMethod::scoreDoc(const QueryRep &qry, lemur::api::DOCID_T docID)
{
	const LinkQueryRep &qRep = *((LinkQueryRep *)(&qry));
	qRep.startIteration();
	
	double score = 0.0;

	if(LinkDistributionParameter::debug)
	{
		std::cout << "scoring doc : " << docID << std::endl;
	}
	assert(LinkDistributionParameter::docWindowSize>0);
	int docCount = ind.docCount();
	int colLength = ind.termCount();
	int x = LinkDistributionParameter::docWindowSize;
	int docLength = ind.docLength(docID);

	while(qRep.hasMore())//遍历query中的link列表
	{
		LinkInfoList *linkInfoList = qRep.nextEntry();	
		int linkCountDoc = linkInfoList->getCount(docID, x);//link在指定文档、指定窗口大小时出现的次数
		double tmp1 = (x-1) * (docLength - (x/2.0));
		double probD = linkCountDoc/tmp1;
		//double probD = 1.0 * linkCountDoc/docLength;
		if(probD < 0)
			probD = abs(probD);

		/*
		if(probD < 0)
		{
			cout << "docLength : " << docLength << endl;
			cout << probD << "\tprobD < 0" << endl;
			exit(1);
		}
		*/
		int linkCountColX = linkInfoList->getCount(x);
		int linkCountCol = linkInfoList->getCount();//link在集合中出现的次数，窗口为无穷大
		double alpha = LinkDistributionParameter::useLinkDist ? 1.0/linkCountCol : 1.0;
		
		double tmp2 = 1.0*(x - 1)*(colLength - docCount*x/2);
		double probC = (alpha * linkCountColX)/tmp2;
		//double probC = 1.0 * (alpha * linkCountColX)/colLength;
		if(probC < 0)
			probC = abs(probC);
		/*
		if(probC < 0)
		{
			cout << "colLength : " << colLength << endl;
			cout << "docCount*x/2 : " << docCount*x/2 << endl; 
			cout << "colLength - docCount*x/2 : " << colLength - docCount*x/2 << endl;
			cout << probC << "\tprobC < 0" << endl;
			exit(1);
		}
		*/

		double eta = lemur::api::ParamGetDouble("eta", 0.75);
	
		double prob = (1-eta) * probD + eta * probC;
		if(prob == 0) continue;//某些link在集合中出现的次数很少，例如2次，在指定窗口为40时，出现次数为0，则prob=0
		score += log(prob);

		switch(LinkDistributionParameter::debug)
		{
		case 1:
			std::cout << ". " ;
			break;
		case 2:
			std::cout << "==============================================================================" << std::endl;
			std::cout << "linkCountDoc : " << linkCountDoc << "\tdocLength : " << docLength << "\tx : " << x << std::endl;
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

	if(LinkDistributionParameter::debug)
	{
		std::cout << std::endl << "score : " << score << std::endl;
	}
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
		cerr<<"docID:"<<docID<<endl;
		double phi = lemur::api::ParamGetDouble("phi", 0);
		double currentScore = (1 - phi) * prevScore + phi * scoreDoc(qry, docID);
		results.PushValue(docID, currentScore);
		//system("pause");
	}//while

}//end scoreDocSet

void LinkDistributionRetMethod::updateQuery(QueryRep &qryRep, const lemur::api::DocIDSet &relDocs)
{

}//end updateQuery

void GetAppParam()
{
	RetrievalParameter::get();
	LinkDistributionParameter::get();
}//end GetAppParam

int AppMain(int argc, char *argv[])
{
	indri::utility::IndriTimer oneQueryTimer;//一个查询检索所需要的时间
	Index *ind;
	try
	{
		ind = IndexManager::openIndex(RetrievalParameter::databaseIndex);//打开索引文件
	}
	catch(Exception &ex)
	{
		ex.writeMessage();
		throw Exception("LinkDistributionRetMethod", "Can't open index, check parameter index");
	}

	DocStream *qryStream;
	try
	{
		qryStream = new lemur::parse::BasicDocStream(RetrievalParameter::textQuerySet);//打开query文件
	}
	catch(Exception &ex)
	{
		ex.writeMessage();
		throw Exception("LinkDistributionRetMethod",
                    "Can't open query file, check parameter textQuery");
	}

	ofstream result(lemur::api::ParamGetString("resultFile","").c_str());//创建输出文件流，如res.simple_okapi

	ResultFile resFile(RetrievalParameter::TRECresultFileFormat);//文件格式
	resFile.openForWrite(result, *ind);

	ifstream *workSetStr;
	ResultFile *docPool;
	if(RetrievalParameter::useWorkingSet)
	{
		workSetStr = new ifstream(RetrievalParameter::workSetFile.c_str(), ios::in);
		if(workSetStr->fail())
		{
			throw Exception("LinkDistributionRetMethod", "can't open working set file");
		}

		docPool = new ResultFile(false);// working set is always simple format
		docPool->openForRead(*workSetStr, *ind);
	}

	lemur::retrieval::ArrayAccumulator accumulator(ind->docCount());//保存集合中每个文档的得分
	IndexedRealVector results(ind->docCount());

	RetrievalMethod *model = new LinkDistributionRetMethod(*ind, accumulator);

	qryStream->startDocIteration();
	LinkQuery *q;

	IndexedRealVector workSetRes;

	while(qryStream->hasMore())//遍历query文件
	{
		Document *d = qryStream->nextDoc();//从query文件中取出一个文档

		q = new LinkQuery(*d, *ind); //根据文档中的term生成link
		cerr << "query : " << q->id() << endl;
		cout << "query : " << q->id() << endl;//输出query中的文档编号

		oneQueryTimer.reset();
		oneQueryTimer.start();
		QueryRep *qr = model->computeQueryRep(*q);//计算link在文档中出现的次数，距离等信息，使用后必须释放内存

		PseudoFBDocs *workSet;
		if(RetrievalParameter::useWorkingSet)
		{
			docPool->getResult(q->id(), workSetRes);
			workSet = new PseudoFBDocs(workSetRes, -1, false);
			model->scoreDocSet(*qr, *workSet, results);
		}
		results.Sort();
		resFile.writeResults(q->id(), &results,RetrievalParameter::resultCount);

		if (RetrievalParameter::useWorkingSet)
		{
		  delete workSet;
		  workSet = NULL;
		}
		oneQueryTimer.stop();
		std::cout << "total execution time(minute:second.microsecond)";
		oneQueryTimer.printElapsedMicroseconds(std::cout);
		std::cout<<endl;
		delete qr;
		qr = NULL;
		delete q;
		q = NULL;
	}//while
	if (RetrievalParameter::useWorkingSet)
	{
		delete docPool;
		docPool = NULL;
		delete workSetStr;
		workSetStr = NULL;
	}
	delete model;
	model = NULL;
	result.close();
	delete qryStream;
	qryStream = NULL;
	delete ind;
	ind = NULL;
	return 0;
}//end AppMain

int main(int argc, char *argv[])
{
	if(argc >= 2 && strcmp(argv[1], "-help") && 
		strcmp(argv[1], "--help") && 
		strcmp(argv[1], "-h"))
	{
		if(!lemur::api::ParamPushFile(argv[1]))
		{
			cerr << "Unable to load parameter file:" << argv[0] << endl;
			exit(1);
		}
	}
	else
	{
		GetAppParam();
		cerr << "Parameters for " << argv[0] << endl;
		lemur::api::ParamDisplay();
		exit(0);
	}

	GetAppParam();//获取参数值

	int result = 1;
	try
	{
		double floorValue = LinkDistributionParameter::floorValue;
		double proofValue = LinkDistributionParameter::proofValue;

		if(LinkDistributionParameter::parameterTuning == 1)//令phi = 1，确定eta参数
		{
			lemur::api::ParamSet("phi", "1.0");
			
			for(int i = 0; i != 9; ++i)
			{
				double tmp = floorValue + (proofValue - floorValue) * (i + 1) / 10.0;
				std::ostringstream outstring;
				outstring << tmp;
				lemur::api::ParamSet("eta", outstring.str());

				string s = RetrievalParameter::resultFile;
				s += "_eta_" + outstring.str();
				lemur::api::ParamSet("resultFile",s);
				
				result = AppMain(argc, argv);
			}
		}
		else if(LinkDistributionParameter::parameterTuning == 2)//确定phi参数
		{		
			for(int i = 0; i != 9; ++i)
			{
				double tmp = floorValue + (proofValue - floorValue) * (i + 1) / 10.0;
				std::ostringstream outstring;
				outstring << tmp;
				lemur::api::ParamSet("phi", outstring.str());

				string s = RetrievalParameter::resultFile;
				s += "_phi_" + outstring.str();
				lemur::api::ParamSet("resultFile",s);
				

				result = AppMain(argc, argv);
			}
		}
		else//正常试验
		{
			result = AppMain(argc, argv);
		}
		
	}
	catch(lemur::api::Exception &ex)
	{
		ex.writeMessage();
		std::cerr << "Program aborted due to exception" << endl;
		exit(1);
	}

	lemur::api::ParamClear();
	return result;
}//end main
