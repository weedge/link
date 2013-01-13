/*==========================================================================
 *date     (do sth)COMMENTS	          author					
 *01/2012   created				        wy

 linkFile format: // modified by wy 2012/02/20
	true:	termID-termID term-term MI CDF  
              ....
	false:  termID-termID MI CDF 
			  ....
 *==========================================================================
*/

#ifndef INVLINKINFOFILE_HPP
#define INVLINKINFOFILE_HPP

#include "common_headers.hpp"
#include "Index.hpp"
#include "IndexedMIandCDF.hpp"
#include "IndexedReal.hpp"
#include "Link.hpp"
#include "KeyfileLinkIndex.hpp"
namespace link
{
	namespace file
	{
		class InvLinkInfoFile
		{
		public:
			InvLinkInfoFile(bool LinkFormat = false) : LinkFmt(LinkFormat){}
			~InvLinkInfoFile(){}

			void openForWrite(lemur::api::Index* index,link::api::KeyfileLinkIndex* linkindex, std::ostream &outfile)
			{
				linkInd = linkindex;
				ind = index;
				outLinkfile = &outfile;
			}
			
			void openForRead(lemur::api::Index* index, link::api::KeyfileLinkIndex* linkindex, std::istream &infile)
			{
				linkInd = linkindex;
				ind = index;
				inLinkfile = &infile;
			}


			void writeLinkResultToFile(link::api::IndexedMIandCDFVector *results);

			void writeMaxValueToFile(link::api::IndexedMIandCDFVector *results)
			{
				double maxMI = results->getMaxValue(MI);
				double maxCDF = results->getMaxValue(CDF);
				*outLinkfile << "maxMI: "<<maxMI<<" maxCDF: "<<maxCDF<<endl;
			}

			/// write the avg Value to the file between 'from' to 'to'.ex:MI[]->avgMI
			void writeAvgValueToFile(link::api::IndexedMIandCDFVector *results)
			{
				int count = 0;
				double maxMI = results->getMaxValue(MI);
				double maxCDF = results->getMaxValue(CDF);

				double avgMI = results->getAvgValue(0.0,maxMI,count,MI);
				 *outLinkfile << "avgMI(0,maxMI): "<<avgMI<<" linkCount:"<<count<<endl;
				double avgCDF = results->getAvgValue(0.0,maxCDF,count,CDF);
				 *outLinkfile << " avgCDF(0,maxCDF): "<<avgCDF<<" linkCount:"<<count<<endl;

				 double i;
				 for (i=1.0;i<=maxMI;i+=1.0)
				 {
					 for (double j=0.0;j<=maxMI;j+=i)
					 {
						 double to = (j+i)>maxMI ? maxMI:(j+i);
						 *outLinkfile << "avgMI("<<j<<"-"<<to<<"]: "<<results->getAvgValue(j,to,count,MI);
						 *outLinkfile <<" linkCount:"<<count<<endl;

					 }
				 }
				 for (i=0.1;i<=maxCDF;i+=0.1)
				 {
					 for (double j=0.0;j<=maxCDF;j+=i)
					 {
						 double to = (j+i)>maxCDF ? maxCDF:(j+i);
						 *outLinkfile << " avgCDF("<<j<<"-"<<to<<"]: "<<results->getAvgValue(j,to,count,CDF);
						 *outLinkfile <<" linkCount:"<<count<<endl;

					 }
				 }

			}

			/// write the avg Value by Real value to the file between 'from' to 'to'. ex: MI[]->avgCDF
			void writeAvgValueByRealToFile(link::api::IndexedMIandCDFVector *results)
			{
				*outLinkfile << "=================avgMIbyCDF & avgCDFbyMI================"<<endl;
				int count = 0;
				double maxMI = results->getMaxValue(MI);
				double maxCDF = results->getMaxValue(CDF);
				double avgMIbyCDF = results->getAvgValue(0.0,maxCDF,count,CDF);
				double avgCDFbyMI = results->getAvgValue(0.0,maxMI,count,MI);
				*outLinkfile << "avgMIbyCDF: "<<avgMIbyCDF<<" avgCDFbyMI: "<<avgCDFbyMI<<endl;

				double i;
				double gapMI = 0.1;//初始以MI间隔0.2的范围求均值，逐步递增。
				double gapCDF = 0.1;//初始以CDF间隔0.1的范围求均值，逐步递增。

				for (i=gapCDF;i<=maxCDF;i+=gapCDF)
				{
					for (double j=0.0;j<=maxCDF;j+=i)
					{
						double to = (j+i)>maxCDF ? maxCDF:(j+i);
						*outLinkfile << "avgMIbyCDF("<<j<<"-"<<to<<"]: "<<results->getAvgValueByReal(j,to,count,CDF);
						*outLinkfile <<" linkCount:"<<count<<endl;

					}
				}

				*outLinkfile<<endl;

				for (i=gapMI;i<=maxMI;i+=gapMI)
				{
					for (double j=0.0;j<=maxMI;j+=i)
					{
						double to = (j+i)>maxMI ? maxMI:(j+i);
						*outLinkfile << "avgCDFbyMI("<<j<<"-"<<to<<"]: "<<results->getAvgValueByReal(j,to,count,MI);
						*outLinkfile <<" linkCount:"<<count<<endl;

					}
				}


			}

			void writeLinkResultToFile(lemur::api::IndexedRealVector *results);

		protected:
			bool LinkFmt;
			std::istream* inLinkfile;
			std::ostream* outLinkfile;
			lemur::api::Index* ind;
			link::api::KeyfileLinkIndex* linkInd;
		private:
		};
	}
}





#endif