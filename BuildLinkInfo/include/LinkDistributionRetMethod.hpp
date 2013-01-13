#ifndef _LINK_DISTRIBUTION_RET_METHOD_HPP
#define _LINK_DISTRIBUTION_RET_METHOD_HPP

#include "RetrievalMethod.hpp"
#include "Param.hpp"
#include "Index.hpp"
#include "KeyfileLinkIndex.hpp"
#include "ScoreAccumulator.hpp"
#include "Link.hpp"
#include "DocumentRep.hpp"
#include "BasicLinkDocStream.hpp"
#include <string>
#include <sstream>

namespace LinkDistributionParameter
{
	static lemur::utility::String databaseLinkIndex;
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
		databaseLinkIndex = lemur::api::ParamGetString("linkIndex","");
		
	}//end get

}//end LinkDistributionParameter


namespace link
{
	namespace retrieval
	{


		class LinkQuery : public lemur::api::Query
		{
		public:
			LinkQuery(){}

			LinkQuery(const lemur::api::Document &doc)
			{
				d = dynamic_cast<const link::parse::BasicLinkDoc*> (&doc);
				if (d->getID()) qid=d->getID(); 
				else qid=""; 
			}

			virtual ~LinkQuery(){}
			virtual void startLinkIteration() const { d->startTermIteration();}
			virtual bool hasMore() const { return d->hasMore();}
			virtual const link::api::Link *nextLink() const { return d->nextLink();}

		protected:
			const link::parse::BasicLinkDoc *d;
		};


		class LinkQueryRep : public lemur::api::QueryRep
		{
		public:
			LinkQueryRep(){}

			virtual ~LinkQueryRep(){};

		private:
		};

		class LinkDistributionRetMethod : public lemur::api::RetrievalMethod
		{
		public:
			/** 构造函数，初始化参数 */
			LinkDistributionRetMethod(const lemur::api::Index &dbindex, lemur::api::ScoreAccumulator &accumulator,
				link::api::KeyfileLinkIndex *linkdbIndex);

			virtual ~LinkDistributionRetMethod();


			/// Score a document identified by the id w.r.t. a query rep
			/// overriding abstract class method
			virtual double scoreDoc(const lemur::api::QueryRep &qry, lemur::api::DOCID_T docID);

			/// compute the representation for a query, semantics defined by subclass
			/// overriding abstract class method
			virtual lemur::api::QueryRep *computeQueryRep(const lemur::api::Query &qry); 

			/// update the query, feedback support
			/// overriding abstract class method
			virtual void updateQuery(lemur::api::QueryRep &qryRep, const lemur::api::DocIDSet &relDocs);

			/// compute the doc representation (caller responsible for deleting the memory of the generated new instance)
			//virtual lemur::api::DocumentRep *computeDocRep(lemur::api::DOCID_T docID);

			void scoreDocSet(const lemur::api::QueryRep &qrp, const lemur::api::DocIDSet &docSet, lemur::api::IndexedRealVector &results);

			void scoreDocSet(const lemur::api::Query &q, const lemur::api::DocIDSet &docSet, lemur::api::IndexedRealVector &results);
			double scoreDoc(const lemur::api::Query &q, lemur::api::DOCID_T docID);

			///// overriding abstract class method with a general efficient inverted index scoring procedure
			virtual void scoreCollection(const lemur::api::QueryRep &qry, lemur::api::IndexedRealVector &results);
			///// add support for scoring an existing document against the collection
			virtual void scoreCollection(lemur::api::DOCID_T docid, lemur::api::IndexedRealVector &results);

		protected:
			link::api::KeyfileLinkIndex *linkIndex;
			lemur::api::ScoreAccumulator &scAcc;
		};



		
	}
}


#endif
