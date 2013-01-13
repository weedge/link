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
 * �����ĵ��÷�
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

	while(qRep.hasMore())//����query�е�link�б�
	{
		LinkInfoList *linkInfoList = qRep.nextEntry();	
		int linkCountDoc = linkInfoList->getCount(docID, x);//link��ָ���ĵ���ָ�����ڴ�Сʱ���ֵĴ���
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
		int linkCountCol = linkInfoList->getCount();//link�ڼ����г��ֵĴ���������Ϊ�����
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
		if(prob == 0) continue;//ĳЩlink�ڼ����г��ֵĴ������٣�����2�Σ���ָ������Ϊ40ʱ�����ִ���Ϊ0����prob=0
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
	indri::utility::IndriTimer oneQueryTimer;//һ����ѯ��������Ҫ��ʱ��
	Index *ind;
	try
	{
		ind = IndexManager::openIndex(RetrievalParameter::databaseIndex);//�������ļ�
	}
	catch(Exception &ex)
	{
		ex.writeMessage();
		throw Exception("LinkDistributionRetMethod", "Can't open index, check parameter index");
	}

	DocStream *qryStream;
	try
	{
		qryStream = new lemur::parse::BasicDocStream(RetrievalParameter::textQuerySet);//��query�ļ�
	}
	catch(Exception &ex)
	{
		ex.writeMessage();
		throw Exception("LinkDistributionRetMethod",
                    "Can't open query file, check parameter textQuery");
	}

	ofstream result(lemur::api::ParamGetString("resultFile","").c_str());//��������ļ�������res.simple_okapi

	ResultFile resFile(RetrievalParameter::TRECresultFileFormat);//�ļ���ʽ
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

	lemur::retrieval::ArrayAccumulator accumulator(ind->docCount());//���漯����ÿ���ĵ��ĵ÷�
	IndexedRealVector results(ind->docCount());

	RetrievalMethod *model = new LinkDistributionRetMethod(*ind, accumulator);

	qryStream->startDocIteration();
	LinkQuery *q;

	IndexedRealVector workSetRes;

	while(qryStream->hasMore())//����query�ļ�
	{
		Document *d = qryStream->nextDoc();//��query�ļ���ȡ��һ���ĵ�

		q = new LinkQuery(*d, *ind); //�����ĵ��е�term����link
		cerr << "query : " << q->id() << endl;
		cout << "query : " << q->id() << endl;//���query�е��ĵ����

		oneQueryTimer.reset();
		oneQueryTimer.start();
		QueryRep *qr = model->computeQueryRep(*q);//����link���ĵ��г��ֵĴ������������Ϣ��ʹ�ú�����ͷ��ڴ�

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

	GetAppParam();//��ȡ����ֵ

	int result = 1;
	try
	{
		double floorValue = LinkDistributionParameter::floorValue;
		double proofValue = LinkDistributionParameter::proofValue;

		if(LinkDistributionParameter::parameterTuning == 1)//��phi = 1��ȷ��eta����
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
		else if(LinkDistributionParameter::parameterTuning == 2)//ȷ��phi����
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
		else//��������
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
