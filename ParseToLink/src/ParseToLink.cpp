/*==========================================================================
 * Copyright (c) 2001 Carnegie Mellon University.  All Rights Reserved.
 *
 * Use of the Lemur Toolkit for Language Modeling and Information Retrieval
 * is subject to the terms of the software license set forth in the LICENSE
 * file included with this software, and also available at
 * http://www.lemurproject.org/license.html
 *
 *==========================================================================
*/
#include "TextHandlerManager.hpp"
#include "IndexManager.hpp"
#include "WriterTextHandler.hpp"
#include "Param.hpp"
#include "indri/Path.hpp"
#include "Exception.hpp"
#include "CandidateLinkFile.hpp"
#include "LinkTypes.hpp"
#include "InvFPDocList.hpp"
#include "BasicDocStream.hpp"
#include "DocStream.hpp"
#include "indri/IndriTimer.hpp"

#include <algorithm>

using namespace lemur::api;
using namespace std;

// Local parameters used by the indexer 
namespace LocalParameter 
{
	static std::string index;
	static std::string textQuerySet;
	static std::string outLinkFile;
	static bool showLink;

	//parse to DocStream format to read
	static bool hasDocFormat;

	void get()
	{
		outLinkFile = ParamGetString("outLinkFile");
		index = ParamGetString("index");
		textQuerySet = ParamGetString("textQuerySet");
		showLink = (ParamGetString("showLink","false")=="true"); //default showLink is false.
		hasDocFormat = (ParamGetString("hasDocFormat","false")=="true"); //default hasDocFormat is false.

	}
};

// get application parameters
void GetAppParam() 
{
	LocalParameter::get();
}

/*½«qryStrem½âÎö³Élink*/
void write_queryLinkInfo(lemur::api::Index* index, lemur::api::DocStream* qryStream,
						 link::file::CandidateLinkFile *linkResultsFile, bool docFormat)
{
	std::vector<lemur::api::TERMID_T> queryTermIDs;

	qryStream->startDocIteration();

	while (qryStream->hasMore())
	{
		queryTermIDs.clear();
		lemur::api::Document* qDoc = qryStream->nextDoc();
		std::cerr<<"Query:"<<qDoc->getID()<<endl;
		if (docFormat)
		{
			linkResultsFile->beginDoc(qDoc->getID());
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
		for (int i=0;  i<qtermNums-1; i++)
		{
			leftTID = queryTermIDs[i];
			for (int j=i+1; j<qtermNums; j++)
			{
				rightTID = queryTermIDs[j];
				link::api::Link pairID(leftTID,rightTID);
				linkResultsFile->writeResults(pairID);
			}

		}
		if(docFormat)
		{
			linkResultsFile->endDoc();
		}	
	}//end while
}

int AppMain(int argc, char * argv[]) 
{
	indri::utility::IndriTimer Timer;
	Timer.start();
	Index* ind;

	try 
	{
		ind = IndexManager::openIndex(LocalParameter::index);//open the inverted index.
	} 
	catch (Exception &ex) 
	{
		ex.writeMessage();
		throw Exception("BuildLinkInfo", "Can't open index, check parameter index");
	}

	DocStream *qryStream;
	try
	{
		qryStream = new lemur::parse::BasicDocStream(LocalParameter::textQuerySet);//open the query file.
	}
	catch (Exception &ex)
	{
		ex.writeMessage(cerr);
		throw Exception("RetEval",
			"Can't open query file, check parameter textQuery");
	}


	std::cerr<<"Parse link to File ~! "<<std::endl;

	char linkFileName[500];
	strcpy(linkFileName, LocalParameter::outLinkFile.c_str());
	if (LocalParameter::hasDocFormat)
	{
		if (LocalParameter::showLink)
		{
			strcat(linkFileName, ".doclinkStr");
		}
		else{
			strcat(linkFileName, ".doclink");
		}
	}else
	{
		if (LocalParameter::showLink)
		{
			strcat(linkFileName, ".dlinkStr");
		}
		else{
			strcat(linkFileName, ".link");
		}
	}

	ofstream linkOFS(linkFileName);

	link::file::CandidateLinkFile linkResultsFile(LocalParameter::showLink);

	linkResultsFile.openForWrite(linkOFS,ind);
	
	write_queryLinkInfo(ind, qryStream, &linkResultsFile, LocalParameter::hasDocFormat);

	linkOFS.close();
	std::cerr<<"Complete~!"<<std::endl;

	Timer.stop();
	std::cout << "total execution time(minute:second.microsecond)";
	Timer.printElapsedMicroseconds(std::cout);
	std::cout << std::endl;

	delete ind;

	return 0;

}

