#include <cassert>
#include <algorithm>
#include <bitset>
#include "common_headers.hpp"
#include "IndexManager.hpp"
#include "Param.hpp"
#include "InvFPDocList.hpp"
#include "indri/IndriTimer.hpp"
#include "Link.hpp"
#include "LinkResultFile.hpp"
#include "Term.hpp"
#include "DocStream.hpp"
#include "BasicDocStream.hpp"
#include "TermInfoList.hpp"

#include "KeyfileLinkIndex.hpp"
#include "IndexedReal.hpp"
#include "IndexedMIandCDF.hpp"

using namespace lemur::api;
using namespace link::file;
using namespace link::api;

#define MAX_BITS 164598 /*根据TOC中的文档数目定义大小 to get the MI or linkDF*/

namespace BuildLinkInfoParameter 
{

	static int debug;//just to print some info to debug.

	// name (minus extension) of the database to open 
	static string index;

	//=============================================
	// name (minus extension) of the linkindex database to open for get CDF
	static string databaseLinkIndex;
	//a windows size in the doc for get CDF
	static int docWindSize;
	// link info in the linkFile for read/write it;
	static std::string linkFile;
	//MI:0, CDF:1
	static int MIorCDF = MI;
	
	//=======================
	// memory to build linkIndex
	static int memory;
	// name (minus extension) of Link database to build
    static string linkIndex;
	// build link index need some windows;
	static int windowCount;
	std::vector<int> windowSizes;

	// candidate link in the candlinkFile for read/write it;
	static std::string candidateLinkFile;
	// 通过MI值设定阈值来得到候选link
	static string MI_threshold  = "0.0";
	// use the DocStream format to read to build candidate link index;
	static bool DocStreamFormat;
	// open to read the candidate link
	static bool usedCandLink;
	static string textQuerySet;
	// show link true:termID-termID, term-term | false: termID-termID   or  print the link info
	static bool showLink;
	// for using the bitset get the candidate link by MI  just to compare with the not use it.
	static bool useBitSet;
	//=========================================

	void get()
	{
		debug = ParamGetInt("debug",0);//default no debug

		index = ParamGetString("index");
		databaseLinkIndex = ParamGetString("linkIndex");
		docWindSize = ParamGetInt("docWindSize", 40);
		linkFile = ParamGetString("linkFile");
		MIorCDF = ParamGetInt("MIorCDF", 0);//default MI:0


		candidateLinkFile = lemur::api::ParamGetString("candidateLinkFile");
		usedCandLink = (ParamGetString("usedCandLink", "false") == "true");//default false.
		useBitSet = (ParamGetString("useBitSet", "false") == "true");//default false.
		DocStreamFormat = (ParamGetString("DocStreamFormat","false")=="true"); //default false.
		MI_threshold = ParamGetString("MIthreshold","0.0");

		textQuerySet = ParamGetString("textQuerySet");
		showLink = (ParamGetString("showLink","false")=="true"); //default showLink is false.
		linkIndex = ParamGetString("linkIndex","linkInd");
		memory = ParamGetInt("memory", 128000000);
		windowCount = ParamGetInt("windowCount", 1);

		std::string temp = lemur::api::ParamGetString("windowSizes", "40");
		std::istringstream stream(temp);

		windowSizes.resize(windowCount);

		for(int i = 0; i != windowCount; ++i)
		{
			stream >> windowSizes[i];
			if(windowSizes[i] <= 0) {
				std::cerr<<"window size is invalid~!"<<std::endl;
				exit(-1);
			}
		}//for
		std::sort(windowSizes.begin(),windowSizes.end());

	}

};

void GetAppParam()
{
	BuildLinkInfoParameter::get();
}

/*print inverted list*/
void print_invertedList(lemur::api::Index* index)
{
	lemur::api::TERMID_T termID;
	for (termID = 1; termID<=index->termCountUnique(); termID++)
	{
		std::cerr<<termID<<std::endl;
		cout<<termID<<"("<<index->term(termID)<<")"<<"\t";
		lemur::api::DocInfoList* doclist = index->docInfoList(termID);
		doclist->startIteration();
		while (doclist->hasMore())
		{
			lemur::api::DocInfo* docinfo = doclist->nextEntry();
			const lemur::api::LOC_T* Pos = docinfo->positions();
			lemur::api::COUNT_T tf = docinfo->termCount();
			cout<<docinfo->docID()<<" "<<tf<<" ";
			if (Pos != NULL)
			{
				cout<<"<";
				for (int i=0; i<tf; i++)
				{
					cout<<Pos[i]<<" ";
				}
				cout<<">"<<" ";
			}
		
		}
		delete doclist;
		doclist = NULL;
		cout<<endl;
	}
}

/*print linkInfo*/
void print_LinkInfo(lemur::api::Index* index)
{
	lemur::api::TERMID_T leftTID,rightTID;
	lemur::api::COUNT_T UniqTermCount = index->termCountUnique();

	for(leftTID = 1; leftTID < UniqTermCount; leftTID++)
	{
		bool havelinkinfolist = false;//have a linkinfo list?
		bool leftIDfirst = true;//the first to out leftTID

		std::cerr<<leftTID<<std::endl;

		lemur::api::DocInfoList* leftTList = index->docInfoList(leftTID);

		for (rightTID = leftTID+1; rightTID <= UniqTermCount; rightTID++)
		{

			
			lemur::api::DocInfoList* rightTList = index->docInfoList(rightTID);

			leftTList->startIteration();
			rightTList->startIteration();

			lemur::api::DocInfo* leftTdocInfo;
			lemur::api::DocInfo* rightTdocInfo;

			lemur::api::DOCID_T leftdocID;
			lemur::api::DOCID_T rightdocID;
			lemur::api::COUNT_T leftTF ;
			lemur::api::COUNT_T rightTF ;
			const lemur::api::LOC_T* leftPos ;
			const lemur::api::LOC_T* rightPos;


			if (leftTList->hasMore() && rightTList->hasMore())
			{
				leftTdocInfo = leftTList->nextEntry();
				rightTdocInfo = rightTList->nextEntry();
			}

			bool end = false;

			bool havelinkinfo = false;//have a linkinfo?
			bool rigthtIDfirst = true;//the first to out rightTID

			while (!end)
			{
				leftdocID = leftTdocInfo->docID();
				rightdocID = rightTdocInfo->docID();

				//leftTF = leftTdocInfo->termCount();
				//rightTF = rightTdocInfo->termCount();
				//leftPos = leftTdocInfo->positions();
				//rightPos = rightTdocInfo->positions();

				if (leftdocID == rightdocID)
				{
					havelinkinfo = true;
					havelinkinfolist = true;
					if (rigthtIDfirst)
					{
						if (leftIDfirst)
						{
							cout<<leftTID<<"("<<index->term(leftTID)<<")";
							leftIDfirst = false;
						}

						cout<<"-"<<rightTID<<"("<<index->term(rightTID)<<") ";
						rigthtIDfirst = false;
					}
					
					cout<<rightdocID<<" ";
					//if (leftPos != NULL || rightPos != NULL)
					//{
					//	cout<<"<";
					//	for (int i=0; i<leftTF; i++)
					//	{
					//		cout<<leftPos[i]<<" ";
					//	}
					//	cout<<";";
					//	for (int j=0; j<rightTF; j++)
					//	{
					//		cout<<rightPos[j]<<" ";
					//	}
					//	cout<<"> ";
					//}
					if (leftTList->hasMore() && rightTList->hasMore())
					{
						leftTdocInfo = leftTList->nextEntry();
						rightTdocInfo = rightTList->nextEntry();
					}else{
						end = true;
					}
				}
				else if (leftdocID > rightdocID)
				{
					if(rightTList->hasMore())
					{
						rightTdocInfo = rightTList->nextEntry();
					}
					else
					{
						end = true;
					}	
				}
				else
				{
					if(leftTList->hasMore())
					{
						leftTdocInfo = leftTList->nextEntry();
					}
					else
					{
						end = true;
					}
				}
			}//end while

			if (havelinkinfo)
			{
				cout<<"@"<<flush;//a link<leftid-rightid> info over
			}
			delete rightTList;
			rightTList = NULL;

		}//end for

		if (havelinkinfolist)
		{
			cout<<"#"<<endl;//some link<leftid-rightids> infos (a linkinfo list) over
		}
		delete leftTList;
		leftTList = NULL;

	}//end for
}
/*print link docinfo by using vector*/
void print_LinkDocInfoVec(lemur::api::Index* ind)
{
	std::map<lemur::api::TERMID_T, std::vector<lemur::api::DOCID_T> > term_DocIDs;
	lemur::api::COUNT_T uniqueTermCount = ind->termCountUnique();

	for(int termID=1; termID<=uniqueTermCount; termID++)
	{//put the docIDs of all termIDs into map

		lemur::index::InvFPDocList *invDocList = (lemur::index::InvFPDocList*)ind->docInfoList(termID);

		invDocList->startIteration();

		while(invDocList->hasMore())
		{//put the docIDs of the termID into vector
			lemur::api::DocInfo *docInfo = invDocList->nextEntry();
			term_DocIDs[termID].push_back(docInfo->docID());
		}

		delete invDocList;
		invDocList = NULL;
	}

	lemur::api::TERMID_T leftTID,rightTID;

	for(leftTID=1; leftTID<uniqueTermCount; leftTID++)
	{
		bool havelinkinfolist = false;//have a linkinfo list?
		bool leftIDfirst = true;//the first to out leftTID

		std::cerr<<leftTID<<std::endl;

		for(rightTID=leftTID+1; rightTID<=uniqueTermCount; rightTID++)
		{
			bool havelinkinfo = false;//have a linkinfo?
			bool rigthtIDfirst = true;//the first to out rightTID

			std::vector<lemur::api::DOCID_T>::iterator leftIter = term_DocIDs[leftTID].begin();
			std::vector<lemur::api::DOCID_T>::iterator rightIter = term_DocIDs[rightTID].begin();
			while(leftIter!=term_DocIDs[leftTID].end() && rightIter!=term_DocIDs[rightTID].end())
			{

				if((*leftIter) == (*rightIter)){

					havelinkinfo = true;
					havelinkinfolist = true;
					if (rigthtIDfirst)
					{
						if (leftIDfirst)
						{
							cout<<leftTID<<"("<<ind->term(leftTID)<<")";
							leftIDfirst = false;
						}

						cout<<"-"<<rightTID<<"("<<ind->term(rightTID)<<") ";
						rigthtIDfirst = false;
					}

					cout<<*leftIter<<" ";
					leftIter++;
					rightIter++;
				}
				else if((*leftIter) > (*rightIter)){
					rightIter++;
				}
				else{
					leftIter++;
				}
			}
			if (havelinkinfo)
			{
				cout<<"@"<<flush;//a link<leftid-rightid> info over
			}

		}//end for

		if (havelinkinfolist)
		{
			cout<<"#"<<endl;//some link<leftid-rightids> infos (a linkinfo list) over
		}
	}//end for
}


/*print linkinfo for query*/
void print_queryLinkInfo(lemur::api::Index* index,lemur::api::DocStream* qryStream)
{
	std::vector<lemur::api::TERMID_T> queryTermIDs;

	qryStream->startDocIteration();

	while (qryStream->hasMore())
	{
		queryTermIDs.clear();
		lemur::api::Document* qDoc = qryStream->nextDoc();
		std::cerr<<"Query:"<<qDoc->getID()<<endl;
		qDoc->startTermIteration();
		while (qDoc->hasMore())
		{
			const lemur::api::Term* qTerm = qDoc->nextTerm();
			
			lemur::api::TERMID_T qtermID = index->term(qTerm->spelling());
			if (qtermID > 0 )
			{
				cerr<<qTerm->spelling()<<" ";
				queryTermIDs.push_back(index->term(qTerm->spelling()));
			}
			else if (qtermID == 0)
			{
				std::cerr<<"(this qTerm:"<<qTerm->spelling()<<" is not in the inverted index)";
			}
			
		}
		std::cerr<<endl;
		sort(queryTermIDs.begin(),queryTermIDs.end());
		std::vector<lemur::api::TERMID_T>::iterator uniqEndIter = std::unique(queryTermIDs.begin(),queryTermIDs.end());
		queryTermIDs.erase(uniqEndIter, queryTermIDs.end());
		
		//lemur::api::TERMID_T* qtermIDS = new lemur::api::TERMID_T[queryTermIDs.size()];

		//for (int i=0; i<queryTermIDs.size(); i++)
		//{
		//	qtermIDS[i] = queryTermIDs[i];
		//}
		
		int qtermNums = queryTermIDs.size(); 
		lemur::api::TERMID_T leftTID, rightTID;

		for(int i=0; i!=qtermNums-1; i++)
		{
			bool havelinkinfolist = false;//have a linkinfo list?
			bool leftIDfirst = true;//the first to out leftTID
			leftTID = queryTermIDs[i];

			std::cerr<<leftTID<<"("<<index->term(leftTID)<<")"<<std::endl;

			lemur::api::DocInfoList* leftTList = index->docInfoList(leftTID);

			for (int j=i+1; j!=qtermNums; j++)
			{
				rightTID = queryTermIDs[j];
				lemur::api::DocInfoList* rightTList = index->docInfoList(rightTID);

				leftTList->startIteration();
				rightTList->startIteration();

				lemur::api::DocInfo* leftTdocInfo;
				lemur::api::DocInfo* rightTdocInfo;

				lemur::api::DOCID_T leftdocID;
				lemur::api::DOCID_T rightdocID;
				lemur::api::COUNT_T leftTF ;
				lemur::api::COUNT_T rightTF ;
				const lemur::api::LOC_T* leftPos ;
				const lemur::api::LOC_T* rightPos;


				if (leftTList->hasMore() && rightTList->hasMore())
				{
					leftTdocInfo = leftTList->nextEntry();
					rightTdocInfo = rightTList->nextEntry();
				}

				bool end = false;

				bool havelinkinfo = false;//have a linkinfo?
				bool rigthtIDfirst = true;//the first to out rightTID

				while (!end)
				{
					leftdocID = leftTdocInfo->docID();
					rightdocID = rightTdocInfo->docID();

					leftTF = leftTdocInfo->termCount();
					rightTF = rightTdocInfo->termCount();
					leftPos = leftTdocInfo->positions();
					rightPos = rightTdocInfo->positions();

					if (leftdocID == rightdocID)
					{
						havelinkinfo = true;
						havelinkinfolist = true;
						if (rigthtIDfirst)
						{
							if (leftIDfirst)
							{
								cout<<leftTID<<"("<<index->term(leftTID)<<")";
								leftIDfirst = false;
							}

							cout<<"-"<<rightTID<<"("<<index->term(rightTID)<<") ";
							rigthtIDfirst = false;
						}

						cout<<rightdocID<<" ";
						if (leftPos != NULL || rightPos != NULL)
						{
							cout<<"<";
							for (int i=0; i<leftTF; i++)
							{
								cout<<leftPos[i]<<" ";
							}
							cout<<";";
							for (int j=0; j<rightTF; j++)
							{
								cout<<rightPos[j]<<" ";
							}
							cout<<"> ";
						}
						if (leftTList->hasMore() && rightTList->hasMore())
						{
							leftTdocInfo = leftTList->nextEntry();
							rightTdocInfo = rightTList->nextEntry();
						}else{
							end = true;
						}
					}
					else if (leftdocID > rightdocID)
					{
						if(rightTList->hasMore())
						{
							rightTdocInfo = rightTList->nextEntry();
						}
						else
						{
							end = true;
						}	
					}
					else
					{
						if(leftTList->hasMore())
						{
							leftTdocInfo = leftTList->nextEntry();
						}
						else
						{
							end = true;
						}
					}
				}//end while

				if (havelinkinfo)
				{
					cout<<"@"<<flush;//a link<leftid-rightid> info over
				}
				delete rightTList;
				rightTList = NULL;

			}//end for

			if (havelinkinfolist)
			{
				cout<<"#"<<endl;//some link<leftid-rightids> infos (a linkinfo list) over
			}
			delete leftTList;
			leftTList = NULL;

		}//end for

	}//end while
}

/*get total link counts in a document*/
int getLinkCount(lemur::api::Index *index, lemur::api::DOCID_T dodID)
{
	lemur::api::TermInfoList* termList;
	int linkcount = 0;
	termList = index->termInfoList(dodID);//unique term
	int termNum = termList->size();
	//cerr<<"docName:"<<index->document(dodID);
	//cerr<<" unique termNum:"<<termNum<<endl;
	
	int *termCounts = new int[termNum];
	memset(termCounts, 0, sizeof(int)*termNum);
	termList->startIteration();
	int i = 0;
	while (termList->hasMore())
	{
		lemur::api::TermInfo* thisTerm = termList->nextEntry();
		lemur::api::TERMID_T termID = thisTerm->termID();
		lemur::api::COUNT_T termCount = thisTerm->count();
		//cerr<<index->term(termID)<<"("<<termID<<"):"<<termCount<<"  ";
		termCounts[i] = termCount;
		i++;

	}
	for (i=0; i!=termNum; i++)
	{//get total link counts in a document
		for (int j=i+1; j!=termNum; j++)
		{
			linkcount += termCounts[i]*termCounts[j];
		}

	}
	//cerr<<endl;

	delete []termCounts;
	termCounts = NULL;
	delete termList;
	termList = NULL;
	return linkcount;
}

/*get all document's linkCounts return*/
lemur::api::COUNT_T getTotalLinkCount(lemur::api::Index *index)
{
	lemur::api::COUNT_T docNum = index->docCount();
	link::api::LINKCOUNT_T allLinkCounts = 0;
	for (int n=1; n<=docNum; n++)
	{
		cout<<"docName:"<<index->document(n)<<"  ";
		lemur::api::COUNT_T linkcount = getLinkCount(index,n);
		cout<<"link count:"<<linkcount<<endl;
		allLinkCounts += linkcount;
	}
	cout<<"all link counts:"<<allLinkCounts<<endl;
	return allLinkCounts;
}
/*get all document's unique linkCounts return*/
lemur::api::COUNT_T getTotalUniqueLinkCount(lemur::api::Index *index)
{
	link::api::LINKCOUNT_T alluniqLinkCounts = 0;
	lemur::api::TERMID_T leftTID,rightTID;
	lemur::api::COUNT_T UniqTermCount = index->termCountUnique();

	for(leftTID = 1; leftTID < UniqTermCount; leftTID++)
	{

		std::cerr<<leftTID<<std::endl;

		lemur::api::DocInfoList* leftTList = index->docInfoList(leftTID);

		for (rightTID = leftTID+1; rightTID <= UniqTermCount; rightTID++)
		{


			lemur::api::DocInfoList* rightTList = index->docInfoList(rightTID);

			leftTList->startIteration();
			rightTList->startIteration();

			lemur::api::DocInfo* leftTdocInfo;
			lemur::api::DocInfo* rightTdocInfo;

			lemur::api::DOCID_T leftdocID;
			lemur::api::DOCID_T rightdocID;



			if (leftTList->hasMore() && rightTList->hasMore())
			{
				leftTdocInfo = leftTList->nextEntry();
				rightTdocInfo = rightTList->nextEntry();
			}

			bool end = false;

			while (!end)
			{
				leftdocID = leftTdocInfo->docID();
				rightdocID = rightTdocInfo->docID();

				if (leftdocID == rightdocID)
				{
					alluniqLinkCounts++;
					break;

					if (leftTList->hasMore() && rightTList->hasMore())
					{
						leftTdocInfo = leftTList->nextEntry();
						rightTdocInfo = rightTList->nextEntry();
					}else{
						end = true;
					}
				}
				else if (leftdocID > rightdocID)
				{
					if(rightTList->hasMore())
					{
						rightTdocInfo = rightTList->nextEntry();
					}
					else
					{
						end = true;
					}	
				}
				else
				{
					if(leftTList->hasMore())
					{
						leftTdocInfo = leftTList->nextEntry();
					}
					else
					{
						end = true;
					}
				}
			}//end while

			delete rightTList;
			rightTList = NULL;

		}//end for

		delete leftTList;
		leftTList = NULL;

	}//end for
	cout<<"all unique link count:"<<alluniqLinkCounts<<endl;
	return alluniqLinkCounts;
}
void  get_totalUniqueLinkCount(lemur::api::Index *index)
{
	lemur::api::IndexedRealVector termDocIDs;

	lemur::api::COUNT_T UniqTermCount = index->termCountUnique();
	for (int i=1; i<=UniqTermCount; i++)
	{
		termDocIDs.PushValue(i,index->docCount(i));
	}
	termDocIDs.Sort(true);

	lemur::api::TERMID_T leftTID,rightTID;
	lemur::api::COUNT_T total_uniqueLinkNum;
	total_uniqueLinkNum = 0;
	IndexedRealVector::iterator iter1 = termDocIDs.begin();
	IndexedRealVector::iterator iter2;
	for (;iter1 != termDocIDs.end(); iter1++)
	{
		leftTID = (*iter1).ind;
		//lemur::api::COUNT_T leftDF = (*iter1).val;

		std::cerr<<leftTID<<"("<<index->term(leftTID)<<")"<<std::endl;

		lemur::api::DocInfoList* leftTList = index->docInfoList(leftTID);

		for (iter2 = iter1+1; iter2 != termDocIDs.end(); iter2++)
		{

			rightTID = (*iter2).ind;
			//lemur::api::COUNT_T rightDF = (*iter2).val;
			lemur::api::DocInfoList* rightTList = index->docInfoList(rightTID);

			leftTList->startIteration();
			rightTList->startIteration();

			lemur::api::DocInfo* leftTdocInfo;
			lemur::api::DocInfo* rightTdocInfo;

			lemur::api::DOCID_T leftdocID;
			lemur::api::DOCID_T rightdocID;
			if (leftTList->hasMore() && rightTList->hasMore())
			{
				leftTdocInfo = leftTList->nextEntry();
				rightTdocInfo = rightTList->nextEntry();
			}
			bool end = false;

			while (!end)
			{
				leftdocID = leftTdocInfo->docID();
				rightdocID = rightTdocInfo->docID();

				if (leftdocID == rightdocID)
				{

					total_uniqueLinkNum++;
					break;

					if (leftTList->hasMore() && rightTList->hasMore())
					{
						leftTdocInfo = leftTList->nextEntry();
						rightTdocInfo = rightTList->nextEntry();
					}else
					{
						end = true;
					}
				}
				else if (leftdocID > rightdocID)
				{
					if(rightTList->hasMore())
					{
						rightTdocInfo = rightTList->nextEntry();
					}
					else
					{
						end = true;
					}	
				}
				else
				{
					if(leftTList->hasMore())
					{
						leftTdocInfo = leftTList->nextEntry();
					}
					else
					{
						end = true;
					}
				}
			}//end while
			
			delete rightTList;
			rightTList = NULL;

		}//end for
		delete leftTList;
		leftTList = NULL;
		cerr<<"total_uniqueLinkNum:"<<total_uniqueLinkNum<<endl;
	}//end for
	cout<<"total_uniqueLinkNum:"<<total_uniqueLinkNum<<endl;
}

/*put the docIDs of the termID into bitset in order to get the linkDF*/
std::bitset<MAX_BITS> pushDoctoBit(lemur::api::TERMID_T termID, lemur::api::Index *ind)
{
	std::bitset<MAX_BITS> DocIDs;
	DocIDs.reset();

	lemur::index::InvFPDocList *invDocList = (lemur::index::InvFPDocList*)ind->docInfoList(termID);
	invDocList->startIteration();

	while(invDocList->hasMore())
	{
		lemur::api::DocInfo *docInfo = invDocList->nextEntry();
		int docID = docInfo->docID(); 
		DocIDs.set(docID,1);//set the pos(docID) of the bitset to 1.
	}

	delete invDocList;
	invDocList = NULL;
	return DocIDs;
}
/*get the linkDF from the bitset*/
lemur::api::COUNT_T getLinkDF(std::bitset<MAX_BITS> leftDocIDs, std::bitset<MAX_BITS> rightDocIDs)
{
	link::api::LINKCOUNT_T linkDF = 0;

	//count the frequency of the link<leftid,rightid>
	leftDocIDs &= rightDocIDs;
	linkDF = leftDocIDs.count();

	return linkDF;
}


/*print PMI(Pointwise Mutual Information) for queryTerm pairs*/
void print_QryPMI(lemur::api::Index *index,lemur::api::DocStream* qryStream,
				  link::api::KeyfileLinkIndex *linkIndex, link::file::InvLinkInfoFile *linkResFile)
{

	lemur::api::IndexedRealVector qryLinkPMIs; 
	double proX, proY, proXY, PMI;
	proXY=proY=proX=PMI=0.0;
	int pos = 0;
	//lemur::api::COUNT_T termcount = index->termCount();
	//lemur::api::COUNT_T linkcount = getTotalLinkCount(index);
	lemur::api::COUNT_T docNum = index->docCount();

	std::vector<lemur::api::TERMID_T> queryTermIDs;
	qryStream->startDocIteration();
	while (qryStream->hasMore())
	{
		queryTermIDs.clear();
		lemur::api::Document* qDoc = qryStream->nextDoc();
		std::cerr<<"Query:"<<qDoc->getID()<<endl;
		qDoc->startTermIteration();
		while (qDoc->hasMore())
		{
			const lemur::api::Term* qTerm = qDoc->nextTerm();

			lemur::api::TERMID_T qtermID = index->term(qTerm->spelling());
			if (qtermID > 0 )
			{
				cerr<<qTerm->spelling()<<" ";
				queryTermIDs.push_back(index->term(qTerm->spelling()));
			}
			else if (qtermID == 0)
			{
				std::cerr<<"(this qTerm:"<<qTerm->spelling()<<" is not in the inverted index)";
			}

		}
		std::cerr<<endl;
		sort(queryTermIDs.begin(),queryTermIDs.end());
		std::vector<lemur::api::TERMID_T>::iterator uniqEndIter = std::unique(queryTermIDs.begin(),queryTermIDs.end());
		queryTermIDs.erase(uniqEndIter, queryTermIDs.end());

		int qtermNums = queryTermIDs.size(); 
		lemur::api::TERMID_T leftTID, rightTID;

		for(int i=0; i!=qtermNums-1; i++)
		{
			bool havelinkinfolist = false;//have a linkinfo list?
			bool leftIDfirst = true;//the first to out leftTID
			leftTID = queryTermIDs[i];
			lemur::api::COUNT_T leftDF = index->docCount(leftTID); 

			std::cerr<<leftTID<<"("<<index->term(leftTID)<<")"<<std::endl;

			lemur::api::DocInfoList* leftTList = index->docInfoList(leftTID);

			for (int j=i+1; j!=qtermNums; j++)
			{
				rightTID = queryTermIDs[j];
				lemur::api::COUNT_T rightDF =  index->docCount(rightTID);
				lemur::api::DocInfoList* rightTList = index->docInfoList(rightTID);

				leftTList->startIteration();
				rightTList->startIteration();

				lemur::api::DocInfo* leftTdocInfo;
				lemur::api::DocInfo* rightTdocInfo;

				lemur::api::DOCID_T leftdocID;
				lemur::api::DOCID_T rightdocID;
				lemur::api::COUNT_T leftTF ;
				lemur::api::COUNT_T rightTF ;


				if (leftTList->hasMore() && rightTList->hasMore())
				{
					leftTdocInfo = leftTList->nextEntry();
					rightTdocInfo = rightTList->nextEntry();
				}

				bool havelinkinfo = false;//have a linkinfo?
				bool rigthtIDfirst = true;//the first to out rightTID

				bool end = false;
				//int all_leftTF = 0;
				//int all_rightTF = 0;
				//int all_linkTF = 0;
				int linkDF = 0;

				while (!end)
				{
					leftdocID = leftTdocInfo->docID();
					rightdocID = rightTdocInfo->docID();

					leftTF = leftTdocInfo->termCount();
					rightTF = rightTdocInfo->termCount();

					if (leftdocID == rightdocID)
					{
						havelinkinfo = true;
						havelinkinfolist = true;
						if (rigthtIDfirst)
						{
							if (leftIDfirst)
							{
								cout<<leftTID<<"("<<index->term(leftTID)<<" df:"<<leftDF<<")";
								leftIDfirst = false;
							}

							cout<<"-"<<rightTID<<"("<<index->term(rightTID)<<" df:"<<rightDF<<") ";
							rigthtIDfirst = false;
						}

						//int docLen = index->docLength(rightdocID);
						//lemur::api::TermInfoList *termList = index->termInfoList(rightdocID);//unique term
						//int termNum = termList->size();
						//delete termList;
						//termList = NULL;
						//cout<<"docName:"<<index->document(rightdocID)<<"("<<rightdocID<<")  length:"<<docLen<<endl;
						
						//proX = leftTF/double(docLen);
						//proY = rightTF/double(docLen);
						////proXY = (leftTF*rightTF)/double(getLinkCount(index,rightdocID));
						//proXY = (leftTF*rightTF)/double(docLen*(docLen-1)/2);
						//PMI = log(proXY/(proX*proY));
						//cout<<"MI:"<<MI<<endl;
						
						//all_leftTF += leftTF;
						//all_rightTF += rightTF;
						//all_linkTF += (leftTF*rightTF);

						linkDF++;
						pos++;

						if (leftTList->hasMore() && rightTList->hasMore())
						{
							leftTdocInfo = leftTList->nextEntry();
							rightTdocInfo = rightTList->nextEntry();
						}else{
							end = true;
						}
					}
					else if (leftdocID > rightdocID)
					{
						if(rightTList->hasMore())
						{
							rightTdocInfo = rightTList->nextEntry();
						}
						else
						{
							end = true;
						}	
					}
					else
					{
						if(leftTList->hasMore())
						{
							leftTdocInfo = leftTList->nextEntry();
						}
						else
						{
							end = true;
						}
					}
				}//end while
				if (havelinkinfo)
				{
					cout<<"linkDF:"<<linkDF<<" ";
					proX = leftDF/double(docNum);
					proY = rightDF/double(docNum);
					proXY = linkDF/double(docNum);
					PMI = log(proXY/(proX*proY));
					//PMI = log( (docNum*linkDF)/(double(leftDF)*double(rightDF)) );
					cout<<"PMI:"<<PMI<<" ";
					cout<<"@"<<flush;//a link<leftid-rightid> info over
					link::api::Link li(leftTID,rightTID);
					link::api::LINKID_T linkID = linkIndex->linkTerm(li.toString());
					if(linkDF!=0&&leftDF!=0&&rightDF!=0)
					{
						qryLinkPMIs.PushValue(linkID,PMI);
					}
					//int docCount = linkIndex->docCount(linkID);
				}
				delete rightTList;
				rightTList = NULL;

			}//end for

			if (havelinkinfolist)
			{
				cout<<"#"<<endl;//some link<leftid-rightids> infos (a linkinfo list) over
			}
			delete leftTList;
			leftTList = NULL;

		}//end for

	}//end while

	qryLinkPMIs.Sort(true);
	
	linkResFile->writeLinkResultToFile(&qryLinkPMIs);
}


/**print PMI(Pointwise Mutual Information) for all Term pairs  */
void print_allPMI(lemur::api::Index *index)
{
	double proX, proY, proXY, PMI;
	proXY=proY=proX=PMI=0.0;
	lemur::api::COUNT_T linkDF = 0;

	lemur::api::COUNT_T docNum = index->docCount();

	lemur::api::COUNT_T UniqTermCount = index->termCountUnique();
	cerr<<"termCountUnique:"<<UniqTermCount<<endl;
	lemur::api::TERMID_T leftTID, rightTID;

	for(leftTID=1; leftTID<UniqTermCount; leftTID++)
	{
		std::cerr<<leftTID<<"("<<index->term(leftTID)<<")"<<std::endl;
		lemur::api::COUNT_T leftDF =  index->docCount(leftTID);
		std::bitset<MAX_BITS> leftDocIDs = pushDoctoBit(leftTID,index);

		for (rightTID=leftTID+1; rightTID<=UniqTermCount; rightTID++)
		{
			lemur::api::COUNT_T rightDF =  index->docCount(rightTID);
			std::bitset<MAX_BITS> rightDocIDs = pushDoctoBit(rightTID,index);

			linkDF = getLinkDF(leftDocIDs,rightDocIDs);
			if (linkDF>0)
			{	
				proX = leftDF/double(docNum);
				proY = rightDF/double(docNum);
				proXY = linkDF/double(docNum);
				PMI = log(proXY/(proX*proY));
				//PMI = log( (docNum*linkDF)/(double(leftDF)*double(rightDF)) );
				if (PMI>0.0 && BuildLinkInfoParameter::debug !=0)
				{
					cout<<leftTID<<"("<<index->term(leftTID)<<" df:"<<leftDF<<")";
					cout<<"-"<<rightTID<<"("<<index->term(rightTID)<<" df:"<<rightDF<<") ";
					cout<<"linkDF:"<<linkDF<<" ";
					cout<<"PMI:"<<PMI<<endl;
				}
				//cout<<"@"<<flush;//a link<leftid-rightid> info over
			}

		}//end for

		cout<<"#"<<endl;//some link<leftid-rightids> infos (a linkinfo list) over

	}//end for

}

/*print_QryCDF for query links*/
void print_QryCDF(link::api::KeyfileLinkIndex *linkindex, lemur::api::Index *index, 
				  lemur::api::DocStream* qryStream, link::file::InvLinkInfoFile *linkResFile)
{
	lemur::api::IndexedRealVector qryLinkCDFs; 
	std::vector<lemur::api::TERMID_T> queryTermIDs;

	qryStream->startDocIteration();
	int pos= 0;
	while (qryStream->hasMore())
	{
		queryTermIDs.clear();
		lemur::api::Document* qDoc = qryStream->nextDoc();
		std::cerr<<"Query:"<<qDoc->getID()<<endl;
		qDoc->startTermIteration();
		while (qDoc->hasMore())
		{
			const lemur::api::Term* qTerm = qDoc->nextTerm();

			lemur::api::TERMID_T qtermID = index->term(qTerm->spelling());
			if (qtermID > 0 )
			{
				cerr<<qTerm->spelling()<<" ";
				queryTermIDs.push_back(index->term(qTerm->spelling()));
			}
			else if (qtermID == 0)
			{
				std::cerr<<"(this qTerm:"<<qTerm->spelling()<<" is not in the inverted index)";
			}

		}
		std::cerr<<endl;
		sort(queryTermIDs.begin(),queryTermIDs.end());
		std::vector<lemur::api::TERMID_T>::iterator uniqEndIter = std::unique(queryTermIDs.begin(),queryTermIDs.end());
		queryTermIDs.erase(uniqEndIter, queryTermIDs.end());

		//lemur::api::TERMID_T* qtermIDS = new lemur::api::TERMID_T[queryTermIDs.size()];

		//for (int i=0; i<queryTermIDs.size(); i++)
		//{
		//	qtermIDS[i] = queryTermIDs[i];
		//}

		int qtermNums = queryTermIDs.size(); 
		lemur::api::TERMID_T leftTID, rightTID;


		for(int i=0; i!=qtermNums-1; i++)
		{
			leftTID = queryTermIDs[i];

			cerr<<leftTID<<"("<<index->term(leftTID)<<")"<<endl;

			for (int j=i+1; j!=qtermNums; j++)
			{
				rightTID = queryTermIDs[j];
				cout<<leftTID<<"("<<index->term(leftTID)<<")-"<<rightTID<<"("<<index->term(rightTID)<<")"<<endl;
				link::api::LINKCOUNT_T linkCountColX = 0;
				link::api::Link li(leftTID,rightTID);
				link::api::LINKID_T linkID = linkindex->linkTerm(li.toString());

				//int docCount =  linkindex->docCount(linkID);
				//cout<<"docCount:"<<docCount<<" ";
				link::api::LINKCOUNT_T *linkCountColXs = linkindex->linkCountX(linkID);
				link::api::LINKCOUNT_T linkCountCol = linkindex->linkCount(linkID);
				std::vector<int> winSizes = linkindex->winSizes();
				if (linkCountColXs!=NULL)
				{
					int i;
					for (i=0; i<linkindex->winCount(); i++)
					{
						if (BuildLinkInfoParameter::docWindSize==winSizes[i])
						{
							linkCountColX = linkCountColXs[i];

							break;
						}
						else
						{
							continue;
						}

					}
					if(i==(linkindex->winCount()))
					{
						cerr<<"BuildLinkInfoParameter::docWindSize is not in the linkIndex,so it is invalid!";
						exit(-1);
					}
					delete linkCountColXs;
					linkCountColXs = NULL;
				}
				cout<<"linkCountColX:"<<linkCountColX<<" linkCountCol:"<<linkCountCol<<" ";
				double cdf =0.0;
				if (linkCountCol!=0&&linkCountColX!=0)
				{
					cdf = linkCountColX/double(linkCountCol);
				}
				cout<<"CDF:"<<cdf<<endl;
				qryLinkCDFs.PushValue(linkID,cdf);
				
				pos++;
				
				
			}//end for

		}//end for

	}//end while
	qryLinkCDFs.Sort(true);

	
	//write the result about linkCDF to the file
	linkResFile->writeLinkResultToFile(&qryLinkCDFs);
}

/*print_QryMIandCDF*/
void print_QryMIandCDF(link::api::KeyfileLinkIndex *linkindex, lemur::api::Index *index, 
				  lemur::api::DocStream* qryStream, link::file::InvLinkInfoFile *linkResFile)
{
	link::api::IndexedMIandCDFVector qryLinkMIandCDFs;
	int docNum = index->docCount();
	std::vector<lemur::api::TERMID_T> queryTermIDs;

	qryStream->startDocIteration();
	int pos= 0;
	while (qryStream->hasMore())
	{
		queryTermIDs.clear();
		lemur::api::Document* qDoc = qryStream->nextDoc();
		std::cerr<<"Query:"<<qDoc->getID()<<endl;
		qDoc->startTermIteration();
		while (qDoc->hasMore())
		{
			const lemur::api::Term* qTerm = qDoc->nextTerm();

			lemur::api::TERMID_T qtermID = index->term(qTerm->spelling());
			if (qtermID > 0 )
			{
				cerr<<qTerm->spelling()<<" ";
				queryTermIDs.push_back(index->term(qTerm->spelling()));
			}
			else if (qtermID == 0)
			{
				std::cerr<<"(this qTerm:"<<qTerm->spelling()<<" is not in the inverted index)";
			}

		}
		std::cerr<<endl;
		sort(queryTermIDs.begin(),queryTermIDs.end());
		std::vector<lemur::api::TERMID_T>::iterator uniqEndIter = std::unique(queryTermIDs.begin(),queryTermIDs.end());
		queryTermIDs.erase(uniqEndIter, queryTermIDs.end());

		//lemur::api::TERMID_T* qtermIDS = new lemur::api::TERMID_T[queryTermIDs.size()];

		//for (int i=0; i<queryTermIDs.size(); i++)
		//{
		//	qtermIDS[i] = queryTermIDs[i];
		//}

		int qtermNums = queryTermIDs.size(); 
		lemur::api::TERMID_T leftTID, rightTID;

		lemur::api::COUNT_T leftDF,rightDF,linkDF;


		for(int i=0; i!=qtermNums-1; i++)
		{
			leftTID = queryTermIDs[i];

			cerr<<leftTID<<"("<<index->term(leftTID)<<")"<<endl;
			leftDF = index->docCount(leftTID);

			for (int j=i+1; j!=qtermNums; j++)
			{
				rightTID = queryTermIDs[j];
				cout<<leftTID<<"("<<index->term(leftTID)<<")-"<<rightTID<<"("<<index->term(rightTID)<<")"<<endl;
				rightDF = index->docCount(rightTID);
				link::api::LINKCOUNT_T linkCountColX = 0;
				link::api::Link li(leftTID,rightTID);
				link::api::LINKID_T linkID = linkindex->linkTerm(li.toString());

				linkDF =  linkindex->docCount(linkID);
				cout<<"leftDF:"<<leftDF<<" rightDF:"<<rightDF<<" linkDocCount:"<<linkDF<<" ";
				
				link::api::LINKCOUNT_T *linkCountColXs = linkindex->linkCountX(linkID);
				link::api::LINKCOUNT_T linkCountCol = linkindex->linkCount(linkID);
				std::vector<int> winSizes = linkindex->winSizes();
				if (linkCountColXs!=NULL)
				{
					int i;
					for (i=0; i<linkindex->winCount(); i++)
					{
						if (BuildLinkInfoParameter::docWindSize==winSizes[i])
						{
							linkCountColX = linkCountColXs[i];

							break;
						}
						else
						{
							continue;
						}

					}
					if(i==(linkindex->winCount()))
					{
						cerr<<"BuildLinkInfoParameter::docWindSize is not in the linkIndex,so it is invalid!";
						exit(-1);
					}
					delete linkCountColXs;
					linkCountColXs = NULL;
				}
				cout<<"linkCountColX="<<BuildLinkInfoParameter::docWindSize<<":"<<linkCountColX<<" linkCountCol:"<<linkCountCol<<" ";
				double cdf = 0.0;
				double mi = 0.0;
				double proX,proY,proXY;
				proX = proY = proXY = 0.0;
				if (linkCountCol!=0&&linkCountColX!=0)
				{
					cdf = linkCountColX/double(linkCountCol);
				}
				if (linkDF!=0&&rightDF!=0&&leftDF!=0)
				{
					proX = leftDF/double(docNum);
					proY = rightDF/double(docNum);
					proXY = linkDF/double(docNum);
					mi = log(proXY/(proX*proY));
					//mi = log( (docNum*linkDF)/(double(leftDF)*double(rightDF)) );
					//mi = log( (docNum*linkDF)/double(leftDF*rightDF) );//bug: -1.#IND double 溢出 int_isnan(double) or int_finite(double)
				}
				cout<<"CDF:"<<cdf<<" MI:"<<mi<<endl;
				qryLinkMIandCDFs.PushValue(linkID,mi,cdf);

				pos++;


			}//end for

		}//end for

	}//end while
	
	//default the MI descendOrder
	qryLinkMIandCDFs.Sort(MI,true);

	int count = 0;

	double maxMI = qryLinkMIandCDFs.getMaxValue(MI);
	double maxCDF = qryLinkMIandCDFs.getMaxValue(CDF);
	cout<<"maxMI"<<maxMI<<"maxCDF:"<<maxCDF<<endl;
	double avgMI = qryLinkMIandCDFs.getAvgValue(0.0,maxMI,count,MI);
	double avgCDF = qryLinkMIandCDFs.getAvgValue(0.0,maxCDF,count,CDF);
	cout<<"avgMI(0,maxMI):"<<avgMI<<" avgCDF(0.0,maxCDF):"<<avgCDF<<endl;


	double avgCDFbyMI = qryLinkMIandCDFs.getAvgValueByReal(0.0,maxCDF,count,MI);
	double avgMIbyCDF = qryLinkMIandCDFs.getAvgValueByReal(0.0,maxMI,count,CDF);
	cout<<"avgCDFbyMI(0,maxCDF):"<<avgCDFbyMI<<" avgMIbyCDF(0.0,maxMI):"<<avgMIbyCDF<<endl;

	//write the result about linkMI&CDF to the file
	linkResFile->writeLinkResultToFile(&qryLinkMIandCDFs);
	linkResFile->writeMaxValueToFile(&qryLinkMIandCDFs);
	linkResFile->writeAvgValueToFile(&qryLinkMIandCDFs);
	linkResFile->writeAvgValueByRealToFile(&qryLinkMIandCDFs);
}

/*open linkIndex*/
link::api::KeyfileLinkIndex* openLinkIndex(const char* indexTOCFile)
{
	link::api::KeyfileLinkIndex *ind;
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
/*print information */
void print_info(lemur::api::Index *ind,lemur::api::DocStream *qryStream)
{
	std::cerr<<"print the inverted index or link informations or MI or CDF ~! "<<std::endl;
	system("pause");

	link::api::KeyfileLinkIndex  *linkInd = NULL;

	try
	{
		linkInd  = openLinkIndex(BuildLinkInfoParameter::databaseLinkIndex.c_str());//打开linkIndex文件
	}
	catch (Exception &ex)
	{
		ex.writeMessage();
		throw Exception("BuildLinkInfo", "Can't open link index, check parameter link index");
	}

	char linkFileName[500];

	if (!BuildLinkInfoParameter::showLink)
	{	
		print_invertedList(ind);
	}
	else
	{
		//print_LinkInfo(ind);
		//print_queryLinkInfo(ind,qryStream);
		//print_linkCount(ind);->getTotalLinkCount(ind);
		//getTotalLinkCount(ind);
		//getTotalUniqueLinkCount(ind);
		get_totalUniqueLinkCount(ind);

		/// maybe use the switch  .. case ..
		if (BuildLinkInfoParameter::MIorCDF==MI)
		{//print_QryPMI
			strcpy(linkFileName, BuildLinkInfoParameter::linkFile.c_str());	
			strcat(linkFileName, ".link_MI");
			ofstream linkOFS(linkFileName);

			link::file::InvLinkInfoFile linkResultsFile(BuildLinkInfoParameter::showLink);
			linkResultsFile.openForWrite(ind,linkInd,linkOFS);
			print_QryPMI(ind,qryStream,linkInd,&linkResultsFile);
			//print_PMI(ind);
			linkOFS.close();
			
		}
		else if (BuildLinkInfoParameter::MIorCDF==CDF)
		{//print_QryCDF
			for (int x=10; x<=100; x+=10)
			{
				memset(linkFileName,0,500*sizeof(char));
				strcpy(linkFileName, BuildLinkInfoParameter::linkFile.c_str());	
				char str[30];
				BuildLinkInfoParameter::docWindSize = x;// a little stupid.
				itoa(BuildLinkInfoParameter::docWindSize,str,10);//not in the c standard lib. u can code by yourself~!

				strcat(linkFileName, ".link_CDF");
				strcat(linkFileName,str);

				ofstream linkOFS(linkFileName);

				link::file::InvLinkInfoFile linkResultsFile(BuildLinkInfoParameter::showLink);
				linkResultsFile.openForWrite(ind,linkInd,linkOFS);
				print_QryCDF(linkInd,ind,qryStream,&linkResultsFile);

				linkOFS.close();
			}
		}
		else
		{// print_QryMIandCDF
			for (int x=10; x<=100; x+=10)
			{
				memset(linkFileName,0,500*sizeof(char));
				strcpy(linkFileName, BuildLinkInfoParameter::linkFile.c_str());	
				char str[30];
				BuildLinkInfoParameter::docWindSize = x;// a little stupid.
				itoa(BuildLinkInfoParameter::docWindSize,str,10);//not in the c standard lib. u can code by yourself~!

				strcat(linkFileName, ".link_MI&CDF");
				strcat(linkFileName,str);

				ofstream linkOFS(linkFileName);

				link::file::InvLinkInfoFile linkResultsFile(BuildLinkInfoParameter::showLink);
				linkResultsFile.openForWrite(ind,linkInd,linkOFS);
				print_QryMIandCDF(linkInd,ind,qryStream,&linkResultsFile);

				linkOFS.close();
			}
		}
	
	}
	if (linkInd!=NULL)
	{
		delete linkInd;
		linkInd = NULL;
	}

}

/*build Link Info by lemur KefileIncIndex*/
void build_linkInfo(lemur::api::Index *ind)
{
	std::cerr<<"Build LinkInfo to File ~! "<<std::endl;
	system("pause");

	link::api::KeyfileLinkIndex* linkIndex = NULL;
	linkIndex = new link::api::KeyfileLinkIndex(BuildLinkInfoParameter::linkIndex,
		BuildLinkInfoParameter::windowSizes, BuildLinkInfoParameter::candidateLinkFile, 
		BuildLinkInfoParameter::usedCandLink, BuildLinkInfoParameter::memory);

	//通过原始的倒排索引中的信息建立link索引。
	if (BuildLinkInfoParameter::usedCandLink)
	{
		linkIndex->addCandLinkInfotoLinkDB(ind);
	}
	else
	{
		linkIndex->addLinkInfotoLinkDB(ind);
	}

	delete linkIndex;
	linkIndex = NULL;
}


/*get Candidate query link by MI*/
void getQryCandidateLinkbyMI(link::api::KeyfileLinkIndex *linkindex, lemur::api::Index *index, 
						  lemur::api::DocStream* qryStream, link::file::CandidateLinkFile *linkCandFile)
{
	double proX, proY, proXY, PMI;
	proXY=proY=proX=PMI=0.0;

	lemur::api::COUNT_T docNum = index->docCount();

	std::vector<lemur::api::TERMID_T> queryTermIDs;
	qryStream->startDocIteration();
	while (qryStream->hasMore())
	{
		queryTermIDs.clear();
		lemur::api::Document* qDoc = qryStream->nextDoc();
		std::cerr<<"Query:"<<qDoc->getID()<<endl;
		if (BuildLinkInfoParameter::DocStreamFormat)
		{
			linkCandFile->beginDoc(qDoc->getID());
		}
		qDoc->startTermIteration();
		while (qDoc->hasMore())
		{
			const lemur::api::Term* qTerm = qDoc->nextTerm();

			lemur::api::TERMID_T qtermID = index->term(qTerm->spelling());
			if (qtermID > 0 )
			{
				cerr<<qTerm->spelling()<<" ";
				queryTermIDs.push_back(index->term(qTerm->spelling()));
			}
			else if (qtermID == 0)
			{
				std::cerr<<"(this qTerm:"<<qTerm->spelling()<<" is not in the inverted index)";
			}

		}
		std::cerr<<endl;

		sort(queryTermIDs.begin(),queryTermIDs.end());
		std::vector<lemur::api::TERMID_T>::iterator uniqEndIter = std::unique(queryTermIDs.begin(),queryTermIDs.end());
		queryTermIDs.erase(uniqEndIter, queryTermIDs.end());


		int qtermNums = queryTermIDs.size();
		lemur::api::TERMID_T leftTID, rightTID;
		for (int i=0; i<qtermNums-1; i++)
		{
			leftTID = queryTermIDs[i];
			for (int j=i+1; j<qtermNums; j++)
			{
				rightTID = queryTermIDs[j];
				link::api::Link li(leftTID,rightTID);
				link::api::LINKID_T linkID = linkindex->linkTerm(li.toString());
				lemur::api::COUNT_T linkDF = linkindex->docCount(linkID);
				lemur::api::COUNT_T leftDF = index->docCount(leftTID);
				lemur::api::COUNT_T rightDF = index->docCount(rightTID);
				if (linkDF!=0&&leftDF!=0&&rightDF!=0)
				{
					proX = leftDF/double(docNum);
					proY = rightDF/double(docNum);
					proXY = linkDF/double(docNum);
					PMI = log(proXY/(proX*proY));
					//PMI = log( (docNum*linkDF)/(double(leftDF)*double(rightDF)) );
				}
				if  ( PMI > atof(BuildLinkInfoParameter::MI_threshold.c_str()) )
				{
					linkCandFile->writeResults(li);
				}

			}
		}// end for
		if (BuildLinkInfoParameter::DocStreamFormat)
		{
			linkCandFile->endDoc();
		}
	}// end while
}

/*get query candidate link by MI or CDF */
void get_qryCandLink(lemur::api::Index *ind,lemur::api::DocStream* qryStream)
{
	std::cerr<<"get query candidate link by MI or CDF ~! "<<std::endl;
	system("pause");

	link::api::KeyfileLinkIndex  *linkInd = NULL;
	try
	{
		linkInd  = openLinkIndex(BuildLinkInfoParameter::databaseLinkIndex.c_str());//打开linkIndex文件
	}
	catch (Exception &ex)
	{
		ex.writeMessage();
		throw Exception("BuildLinkInfo", "Can't open link index, check parameter link index");
	}

	link::file::CandidateLinkFile candLinkFile(BuildLinkInfoParameter::showLink);

	char clFileName[500];
	strcpy(clFileName, BuildLinkInfoParameter::candidateLinkFile.c_str());
	if(BuildLinkInfoParameter::DocStreamFormat==true)
	{
		strcat(clFileName, ".doccl_threshold");
	}
	else
	{
		strcat(clFileName, ".cl_threshold");
	}
	strcat(clFileName, BuildLinkInfoParameter::MI_threshold.c_str());

	ofstream clOFS;
	clOFS.open(clFileName);

	candLinkFile.openForWrite(clOFS,ind);

	getQryCandidateLinkbyMI(linkInd,ind,qryStream,&candLinkFile);

	clOFS.close();


	delete linkInd;
	linkInd = NULL;
}

/*else write the candidate link to the file for the all candidate links(term pairs) by MI>threshold*/
void getCandLinkByMI(lemur::api::Index *index, link::file::CandidateLinkFile *candLinkFile)
{
	double proX, proY, proXY, PMI;
	proXY=proY=proX=PMI=0.0;
	lemur::api::COUNT_T linkDF = 0;
	lemur::api::COUNT_T candLinkNumByMI = 0;
	lemur::api::COUNT_T total_candLinkNumByMI = 0;
	lemur::api::COUNT_T total_uniqueLinkNum = 0;

	lemur::api::COUNT_T docNum = index->docCount();

	lemur::api::COUNT_T UniqTermCount = index->termCountUnique();
	cerr<<"termCountUnique:"<<UniqTermCount<<endl;
	lemur::api::TERMID_T leftTID, rightTID;

	for(leftTID=1; leftTID<UniqTermCount; leftTID++)
	{
		candLinkNumByMI = 0;
		std::cerr<<leftTID<<"("<<index->term(leftTID)<<")"<<std::endl;
		lemur::api::COUNT_T leftDF =  index->docCount(leftTID);
		std::bitset<MAX_BITS> leftDocIDs = pushDoctoBit(leftTID,index);

		for (rightTID=leftTID+1; rightTID<=UniqTermCount; rightTID++)
		{
			lemur::api::COUNT_T rightDF =  index->docCount(rightTID);
			std::bitset<MAX_BITS> rightDocIDs = pushDoctoBit(rightTID,index);

			linkDF = getLinkDF(leftDocIDs,rightDocIDs);
			if (linkDF>0)
			{	
	
				total_uniqueLinkNum++;
				proX = leftDF/double(docNum);
				proY = rightDF/double(docNum);
				proXY = linkDF/double(docNum);
				PMI = log(proXY/(proX*proY));
				//PMI = log( (docNum*linkDF)/(double(leftDF)*double(rightDF)) );
				//PMI = log( (docNum*linkDF)/(double(leftDF*rightDF)) );
				if ( PMI > atof(BuildLinkInfoParameter::MI_threshold.c_str()) )
				{
					if (candLinkFile!=NULL)
					{
						link::api::Link li(leftTID,rightTID);
						candLinkFile->writeResults(li);
					}
					if (BuildLinkInfoParameter::debug==2)
					{
						cout<<leftTID<<"("<<index->term(leftTID)<<" df:"<<leftDF<<")";
						cout<<"-"<<rightTID<<"("<<index->term(rightTID)<<" df:"<<rightDF<<") ";
						cout<<"linkDF:"<<linkDF<<" ";
						cout<<"PMI:"<<PMI<<endl;
					}
					candLinkNumByMI++;
					total_candLinkNumByMI++;

				}
			}

		}//end for
		if (BuildLinkInfoParameter::debug==1)
		{
			cout<<leftTID<<"("<<index->term(leftTID)<<")";
			cout<<"candLinkNumByMI:"<<candLinkNumByMI<<endl;//some link<leftid-rightids> infos (a linkinfo list) over
		}

	}//end for
	cout<<"total_candLinkNumByMI:"<<total_candLinkNumByMI<<endl;
	cout<<"total_uniqueLinkNum:"<<total_uniqueLinkNum<<endl;
	cout<<"total_candLinkNumByMI/total_uniqueLinkNum:"<<total_candLinkNumByMI/total_uniqueLinkNum<<endl;
}


/*get the candidate link by MI , sort the termID by docCount*/
void  getCandLinkByMI_noUseBitSet(lemur::api::Index *index, link::file::CandidateLinkFile *candLinkFile)
{
	lemur::api::IndexedRealVector termDocIDs;
	double proX, proY, proXY, PMI;
	proXY=proY=proX=PMI=0.0;
	lemur::api::COUNT_T docNum = index->docCount();
	lemur::api::COUNT_T UniqTermCount = index->termCountUnique();
	for (int i=1; i<=UniqTermCount; i++)
	{
		termDocIDs.PushValue(i,index->docCount(i));
	}
	termDocIDs.Sort(true);

	lemur::api::TERMID_T leftTID,rightTID;
	lemur::api::COUNT_T candLinkNumByMI,total_candLinkNumByMI,total_uniqueLinkNum;
	total_uniqueLinkNum = total_candLinkNumByMI = 0;
	IndexedRealVector::iterator iter1 = termDocIDs.begin();
	IndexedRealVector::iterator iter2;
	for (;iter1 != termDocIDs.end(); iter1++)
	{
		candLinkNumByMI = 0;
		leftTID = (*iter1).ind;
		//lemur::api::COUNT_T leftDF = (*iter1).val;
		lemur::api::COUNT_T leftDF = index->docCount(leftTID); 

		std::cerr<<leftTID<<"("<<index->term(leftTID)<<")"<<std::endl;

		lemur::api::DocInfoList* leftTList = index->docInfoList(leftTID);

		for (iter2 = iter1+1; iter2 != termDocIDs.end(); iter2++)
		{

			rightTID = (*iter2).ind;
			//lemur::api::COUNT_T rightDF = (*iter2).val;
			lemur::api::COUNT_T rightDF =  index->docCount(rightTID);
			lemur::api::DocInfoList* rightTList = index->docInfoList(rightTID);

			leftTList->startIteration();
			rightTList->startIteration();

			lemur::api::DocInfo* leftTdocInfo;
			lemur::api::DocInfo* rightTdocInfo;

			lemur::api::DOCID_T leftdocID;
			lemur::api::DOCID_T rightdocID;
			if (leftTList->hasMore() && rightTList->hasMore())
			{
				leftTdocInfo = leftTList->nextEntry();
				rightTdocInfo = rightTList->nextEntry();
			}
			bool end = false;

			int linkDF = 0;

			while (!end)
			{
				leftdocID = leftTdocInfo->docID();
				rightdocID = rightTdocInfo->docID();

				if (leftdocID == rightdocID)
				{

					linkDF++;

					if (leftTList->hasMore() && rightTList->hasMore())
					{
						leftTdocInfo = leftTList->nextEntry();
						rightTdocInfo = rightTList->nextEntry();
					}else
					{
						end = true;
					}
				}
				else if (leftdocID > rightdocID)
				{
					if(rightTList->hasMore())
					{
						rightTdocInfo = rightTList->nextEntry();
					}
					else
					{
						end = true;
					}	
				}
				else
				{
					if(leftTList->hasMore())
					{
						leftTdocInfo = leftTList->nextEntry();
					}
					else
					{
						end = true;
					}
				}
			}//end while
			if (linkDF>0)
			{
				total_uniqueLinkNum++;
				proX = leftDF/double(docNum);
				proY = rightDF/double(docNum);
				proXY = linkDF/double(docNum);
				PMI = log(proXY/(proX*proY));
				//PMI = log( (docNum*linkDF)/(double(leftDF)*double(rightDF)) );
				if ( PMI > atof(BuildLinkInfoParameter::MI_threshold.c_str()) )
				{
					if (candLinkFile!=NULL)
					{
						link::api::Link li(leftTID,rightTID);
						candLinkFile->writeResults(li);
					}
					if (BuildLinkInfoParameter::debug==2)
					{
						cout<<leftTID<<"("<<index->term(leftTID)<<" df:"<<leftDF<<")";
						cout<<"-"<<rightTID<<"("<<index->term(rightTID)<<" df:"<<rightDF<<") ";
						cout<<"linkDF:"<<linkDF<<" ";
						cout<<"PMI:"<<PMI<<endl;
					}
					candLinkNumByMI++;
					total_candLinkNumByMI++;

				}
			}
			delete rightTList;
			rightTList = NULL;

		}//end for
		if (BuildLinkInfoParameter::debug==1)
		{
			cout<<leftTID<<"("<<index->term(leftTID)<<")";
			cout<<"candLinkNumByMI:"<<candLinkNumByMI<<endl;//some link<leftid-rightids> infos (a linkinfo list) over
		}
		delete leftTList;
		leftTList = NULL;
	}//end for
	cout<<"total_candLinkNumByMI:"<<total_candLinkNumByMI<<endl;
	cout<<"total_uniqueLinkNum:"<<total_uniqueLinkNum<<endl;
	cout<<"total_candLinkNumByMI/total_uniqueLinkNum:"<<total_candLinkNumByMI/total_uniqueLinkNum<<endl;
}
/*get candidate link by MI or CDF */
void get_candLink(lemur::api::Index *ind)
{
	std::cerr<<"get collection(TREC) candidate link by MI or CDF ~! "<<std::endl;
	system("pause");

	link::file::CandidateLinkFile candLinkFile(BuildLinkInfoParameter::showLink);

	char clFileName[500];
	strcpy(clFileName, BuildLinkInfoParameter::candidateLinkFile.c_str());
	if(BuildLinkInfoParameter::DocStreamFormat==true)
	{
		strcat(clFileName, ".doccl_threshold");
	}
	else
	{
		strcat(clFileName, ".cl_threshold");
	}
	strcat(clFileName, BuildLinkInfoParameter::MI_threshold.c_str());

	ofstream clOFS;
	clOFS.open(clFileName);

	candLinkFile.openForWrite(clOFS,ind);

	if (BuildLinkInfoParameter::useBitSet)
	{
		getCandLinkByMI(ind,&candLinkFile);
	}
	else
	{
		getCandLinkByMI_noUseBitSet(ind,&candLinkFile);
	}
	

	clOFS.close();

}


int AppMain(int argc, char *argv[])
{

	indri::utility::IndriTimer Timer;


	Index* ind = NULL;

	try 
	{
		ind = IndexManager::openIndex(BuildLinkInfoParameter::index);//open the inverted index.
	} 
	catch (Exception &ex) 
	{
		ex.writeMessage();
		throw Exception("BuildLinkInfo", "Can't open index, check parameter index");
	}

	DocStream *qryStream = NULL;


	cerr<<"options: \n0.print information.\n1.build linkInfo. \n2.get query candidate link. \n3.get candidate link from collection(TREC). \nother:do nothing!"<<endl;
	int opt;
	cin>>opt;
	Timer.start();
	switch (opt)
	{
	case 0:
		try
		{
			qryStream = new lemur::parse::BasicDocStream(BuildLinkInfoParameter::textQuerySet);//open the query file.
		}
		catch (Exception &ex)
		{
			ex.writeMessage(cerr);
			throw Exception("BuildLinkInfo",
				"Can't open query file, check parameter textQuery");
		}
		print_info(ind,qryStream);
		break;

	case 1:
		build_linkInfo(ind);
		break;

	case 2:
		try
		{
			qryStream = new lemur::parse::BasicDocStream(BuildLinkInfoParameter::textQuerySet);//open the query file.
		}
		catch (Exception &ex)
		{
			ex.writeMessage(cerr);
			throw Exception("BuildLinkInfo",
				"Can't open query file, check parameter textQuery");
		}
		get_qryCandLink(ind,qryStream);
		break;

	case 3:
		get_candLink(ind);
		break;

	default:
		break;
	}
	Timer.stop();
	std::cout << "total execution time(minute:second.microsecond)";
	Timer.printElapsedMicroseconds(std::cout);
	std::cout << std::endl;

	if (ind!=NULL)
	{
		delete ind;
		ind = NULL;
	}
	if (qryStream!=NULL)
	{
		delete qryStream;
		qryStream = NULL;
	}

	std::cerr<<"Complete~!"<<std::endl;
	return 0;

}
