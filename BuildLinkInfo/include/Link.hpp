#ifndef LINK_HPP
#define LINK_HPP

#include "IndexTypes.hpp"
#include "LinkTypes.hpp"
#include <sstream>
#include <cstring>
#include <cstdlib>
using namespace std;

namespace link{

	namespace api{
	
		class Link
		{
			/*
			* link<termID, termID> 
			*/
		public:
			Link() : leftID(0), rightID(0), distance(0){}
			Link(const lemur::api::TERMID_T &leftid, const lemur::api::TERMID_T &rightid)
			{
				distance=0;
				if(leftid <= rightid)
				{
					this->leftID = leftid;
					this->rightID = rightid;
				}
				else
				{
					this->leftID = rightid;
					this->rightID = leftid;
				}
				this->linkProcess();
			}
			Link(const Link &link)
			{
				this->leftID = link.getLeftID();
				this->rightID = link.getRightID();
				this->distance = link.getDistance();
				this->linkString = link.toString();
			}

			virtual ~Link(){}

			const lemur::api::TERMID_T getLeftID() const {return leftID;}
			const lemur::api::TERMID_T getRightID() const {return rightID;}
			const link::api::DIST_T getDistance() const {return distance;}

			void setLeftID(const lemur::api::TERMID_T &leftid) {leftID = leftid;}
			void setRightID(const lemur::api::TERMID_T &rightid) {rightID = rightid;}
			void setDistance(const link::api::DIST_T &dist) {distance = dist;}

			link::api::LINK_STR toString() const {return linkString;}
			void setLinkStr(const link::api::LINK_STR &str) {linkString = str;}

			/** 对link进行处理，pair <termid,termid> -> char* termid-termid */
			void linkProcess()
			{
				std::ostringstream stream;
				stream << leftID << "-" << rightID;
				this->linkString = stream.str();
			}

			/** termid-termid -> termid, termid */
			void linkToPair(const link::api::LINK_STR str_link)
			{
				unsigned int loc = str_link.find_first_of('-',0);
				
				leftID = atoi( str_link.substr(0,loc).c_str() );
				rightID = atoi( str_link.substr(loc+1).c_str() );
			}
			

		protected:
			lemur::api::TERMID_T leftID;
			lemur::api::TERMID_T rightID;
			link::api::LINK_STR linkString;//"leftID-rightID"
			link::api::DIST_T distance;//link中term之间的距离
		};
	}
}
#endif
