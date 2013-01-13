/**======================================
/*
 * NAME DATE - COMMENTS
 * wy  02/2012 - created
 *========================================================================*/
/*! \page LinkDistributionRetEval Retrieval Evaluation Application*/
#include "common_headers.hpp"
#include "IndexManager.hpp"
#include "RetParamManager.hpp"
#include "BasicLinkDocStream.hpp"
#include "ResultFile.hpp"
#include "LinkDistributionRetMethod.hpp"
#include "KeyfileLinkIndex.hpp"
#include "Index.hpp"
#include "indri/IndriTimer.hpp"

using namespace lemur::api;
using namespace link::retrieval;
using namespace link::api;

void GetAppParam()
{
    RetrievalParameter::get();
	LinkDistributionParameter::get();
}

link::api::KeyfileLinkIndex* openLinkIndex(const char* indexTOCFile)
{
	KeyfileLinkIndex *ind;
	int len = strlen(indexTOCFile);
	if (!(len>4 && indexTOCFile[len-9]=='.')) {
		; // it must have format .xxxx_xxx 
		std::cerr<<"Index file must have format .xxxx_xxx"<<std::endl;
	}

	const char *extension = &(indexTOCFile[len-8]);
	if ((!strcmp(extension, "link_key")) ||
		(!strcmp(extension, "LINK_KEY"))) {
			ind = new link::api::KeyfileLinkIndex();
	} else {
		std::cerr<<"unknown index file extension"<<std::endl;
	}
	if (!(ind->open(indexTOCFile))) {
		std::cerr<<"open linkindex failed"<<std::endl;
	}
	return ind;
}

/// A retrieval evaluation program
int AppMain(int argc, char *argv[])
{
  assert(LinkDistributionParameter::docWindowSize>0);
  indri::utility::IndriTimer oneQueryTimer;//һ����ѯ��������Ҫ��ʱ��
  KeyfileLinkIndex  *linkind;
  try
  {
    linkind  = openLinkIndex(LinkDistributionParameter::databaseLinkIndex);//��linkIndex�ļ�
  }
  catch (Exception &ex)
  {
    ex.writeMessage();
    throw Exception("LinkDistributionRetrieval", "Can't open link index, check parameter link index");
  }
  Index  *ind;
  try
  {
	  ind  = IndexManager::openIndex(RetrievalParameter::databaseIndex);//��Index�ļ�
  }
  catch (Exception &ex)
  {
	  ex.writeMessage();
	  throw Exception("LinkDistributionRetrieval", "Can't open index, check parameter index");
  }
  DocStream *qryStream;
  try
  {
	  qryStream = new link::parse::BasicLinkDocStream(RetrievalParameter::textQuerySet);//��query�ļ�
  }

  catch(Exception &ex)
  {
	  ex.writeMessage();
	  throw Exception("LinkDistributionRetrieval",
		  "Can't open query file, check parameter textQuery");
  }
  assert(LinkDistributionParameter::docWindowSize>0);
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

  //RetrievalMethod *model = new LinkDistributionRetMethod(*ind, accumulator, linkind);
  LinkDistributionRetMethod *model = new LinkDistributionRetMethod(*ind, accumulator, linkind);

  qryStream->startDocIteration();
  LinkQuery *q;

  IndexedRealVector workSetRes;

  while(qryStream->hasMore())//����query�ļ�
  {
	  
	  Document *d = qryStream->nextDoc();//��query�ļ���ȡ��һ���ĵ�

	  q = new LinkQuery(*d); //�����ĵ��е�term����link

	  cerr << "query : " << q->id() << endl;
	  cout << "query : " << q->id() << endl;//���query�е��ĵ����

	  //QueryRep *qr = model->computeQueryRep(*q);//����link���ĵ��г��ֵĴ������������Ϣ��ʹ�ú�����ͷ��ڴ�
	  oneQueryTimer.reset();
	  oneQueryTimer.start();
	  PseudoFBDocs *workSet;
	  if(RetrievalParameter::useWorkingSet)
	  {
		  docPool->getResult(q->id(), workSetRes);
		  workSet = new PseudoFBDocs(workSetRes, -1, false);
		  //model->scoreDocSet(*qr, *workSet, results);
		  model->scoreDocSet(*q, *workSet, results);
	  }
	  else
	  {
		  //model->scoreCollection(qr,results);
	  }
	  oneQueryTimer.stop();
	  std::cout << "total execution time(minute:second.microsecond)";
	  oneQueryTimer.printElapsedMicroseconds(std::cout);
	  std::cout<<endl;

	  results.Sort();
	  resFile.writeResults(q->id(), &results,RetrievalParameter::resultCount);

	  if (RetrievalParameter::useWorkingSet)
	  {
		  delete workSet;
		  workSet = NULL;
	  }

	  //delete qr;
	  //qr = NULL;
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
  delete linkind;
  linkind = NULL;
  return 0;
}

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
				string s = RetrievalParameter::resultFile;
				if (LinkDistributionParameter::useLinkDist == 0)
				{
					s += "_NOcumulative";
				}
				else
				{
					s += "_cumulative";
				}

				std::ostringstream outstring;
				outstring << tmp;
				lemur::api::ParamSet("eta", outstring.str());

				s += "_phi_1.0_eta_" + outstring.str();
				lemur::api::ParamSet("resultFile",s);

				result = AppMain(argc, argv);
			}
		}
		else if(LinkDistributionParameter::parameterTuning == 2)//ȷ��phi����
		{
			double eta = lemur::api::ParamGetDouble("eta", 0);
			std::ostringstream outstring2;
			outstring2 << eta;

			for(int i = 0; i != 9; ++i)
			{
				double tmp = floorValue + (proofValue - floorValue) * (i + 1) / 10.0;
				string s = RetrievalParameter::resultFile;
				if (LinkDistributionParameter::useLinkDist == 0)
				{
					s += "_NOcumulative";
				}
				else
				{
					s += "_cumulative";
				}

				std::ostringstream outstring1;
				outstring1 << tmp;
				lemur::api::ParamSet("phi", outstring1.str());

				s += "_phi_" + outstring1.str();
				s += "_eta_" + outstring2.str();
				lemur::api::ParamSet("resultFile",s);


				result = AppMain(argc, argv);
			}
		}
		else//��������
		{
			string s = RetrievalParameter::resultFile;
			if (LinkDistributionParameter::useLinkDist == 0)
			{
				s += "_NOcumulative";
			}
			else
			{
				s += "_cumulative";
			}
			double phi = lemur::api::ParamGetDouble("phi", 0);
			std::ostringstream outstring1;
			outstring1 << phi;

			s += "_phi_" + outstring1.str();

			double eta = lemur::api::ParamGetDouble("eta", 0);
			std::ostringstream outstring2;
			outstring2 << eta;
			s += "_eta_" + outstring2.str();
			lemur::api::ParamSet("resultFile",s);

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