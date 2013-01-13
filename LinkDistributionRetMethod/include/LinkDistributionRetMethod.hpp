#ifndef _LINK_DISTRIBUTION_RET_METHOD_HPP
#define _LINK_DISTRIBUTION_RET_METHOD_HPP

#include "RetrievalMethod.hpp"
#include "Param.hpp"
#include "Index.hpp"
#include "ScoreAccumulator.hpp"
#include "Link.hpp"
#include "DocumentRep.hpp"
#include <string>
#include <sstream>

namespace LinkDistributionParameter
{
	static int debug = 0;
	static int parameterTuning;
	static int useLinkDist;

	static double floorValue;
	static double proofValue;
	static double phi;
	static double eta;

	static int queryWindowSize;
	static int docWindowMax;
	static int docWindowCount;
	static int docWindowSize;
	//static vector<int> docWindowSize;
	//static vector<double> docWindowWeights;

	static double defaultFloorValue = 0.0;
	static double defaultProofValue = 1.0;
	static double defaultPhi = 1.0;
	static double defaultEta = 0.0;

	static int defaultQueryWindowSize = 15;
	static int defaultDocWindowMax = 100;
	static int defaultDocWindowCount = 1;
	static int defaultDocWindowSize = 40;

	static void get()
	{
		debug = lemur::api::ParamGetInt("debug", 0);
		parameterTuning = lemur::api::ParamGetInt("parameterTuning", 0);
		floorValue = lemur::api::ParamGetDouble("floorValue", defaultFloorValue);
		proofValue = lemur::api::ParamGetDouble("proofValue", defaultProofValue);
		phi = lemur::api::ParamGetDouble("phi", defaultPhi);
		eta = lemur::api::ParamGetDouble("eta", defaultEta);

		queryWindowSize = lemur::api::ParamGetInt("queryWindowSize", defaultQueryWindowSize);
		docWindowMax = lemur::api::ParamGetInt("docWindowMax", defaultDocWindowMax);
		docWindowCount = lemur::api::ParamGetInt("docWindowCount", defaultDocWindowCount);
		docWindowSize = lemur::api::ParamGetInt("docWindowSize", defaultDocWindowSize);
		useLinkDist = lemur::api::ParamGetInt("useLinkDist", 1);
		
		/**
		std::string sizeStr = lemur::api::ParamGetString("docWindowSize", "40");
		std::string weightsStr = lemur::api::ParamGetString("docWindowWeights", "1");

		std::istringstream sizeStream(sizeStr);
		std::istringstream weightStream(weightsStr);

		std::string size;
		std::string weight;
		
		while(sizeStream >> size)
		{
			docWindowSize.push_back(atoi(size.c_str()));
		}
		
		while(weightStream >> weight)
		{
			docWindowWeights.push_back(atof(weight.c_str()));
		}

		if(docWindowCount != docWindowSize.size() || docWindowCount != docWindowWeights.size())
		{
			cerr << "docWindowCount 和 DocWindowSize、docWindowWeights 参数不匹配" << endl;
			exit(1);
		}
		*/
		
	}//end get

}//end LinkDistributionParameter

class LinkDistributionRetMethod : public lemur::api::RetrievalMethod
{
public:
	/** 构造函数，初始化参数 */
	LinkDistributionRetMethod(const lemur::api::Index &dbindex, lemur::api::ScoreAccumulator &accumulator);
	virtual ~LinkDistributionRetMethod();

	/// overriding abstract class method
	virtual lemur::api::QueryRep *computeQueryRep(const lemur::api::Query &qry); 

	/// compute the doc representation (caller responsible for deleting the memory of the generated new instance)
	//virtual lemur::api::DocumentRep *computeDocRep(lemur::api::DOCID_T docID);

	/// Score a set of documents w.r.t. a query rep (e.g. for re-ranking)
	/*! 
	  The default implementation provided by this class is to call function scoreDoc
	*/
	virtual void scoreDocSet(const lemur::api::QueryRep &qry, const lemur::api::DocIDSet &docSet, lemur::api::IndexedRealVector &results);

	/// overriding abstract class method
	virtual double scoreDoc(const lemur::api::QueryRep &qry, lemur::api::DOCID_T docID);

	/// overriding abstract class method with a general efficient inverted index scoring procedure
	virtual void scoreCollection(const lemur::api::QueryRep &qry, lemur::api::IndexedRealVector &results);
	/// add support for scoring an existing document against the collection
	virtual void scoreCollection(lemur::api::DOCID_T docid, lemur::api::IndexedRealVector &results);

	/// update the query, feedback support
	virtual void updateQuery(lemur::api::QueryRep &qryRep, const lemur::api::DocIDSet &relDocs);

	/// This, along with hasMore(), nextLink(), supports iteration over terms
	//virtual void startIteration() const;
	//virtual bool hasMore() const;

	/// Fetch the next Link. A new instance is generated; the caller is responsible for deleting it!
	//virtual Link *nextLink() const;

	/// Any query-specific constant term in the scoring formula
	//virtual double scoreConstant() const;

protected:
	lemur::api::ScoreAccumulator &scAcc;
};

#endif
