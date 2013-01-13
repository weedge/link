/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 02/2011 - created 
 *========================================================================*/

#ifndef LINK_INFO_HPP
#define LINK_INFO_HPP

#include "IndexTypes.hpp"
#include "LinkTypes.hpp"
#include "Link.hpp"
#include <string>
#include <vector>

namespace link{
	namespace api{

		/**
		 * 描述link在文档中出现的次数，距离等信息
		 */
		class LinkInfo : public link::api::Link
		{
		public:

			LinkInfo();
			LinkInfo(const lemur::api::TERMID_T leftid, const lemur::api::TERMID_T rightid);

			virtual ~LinkInfo();

			virtual void setLinkID(link::api::LINKID_T &linkid)
			{
				linkID = linkid;
			}

			virtual link::api::LINKID_T getLinkID() const {return linkID;}

			virtual void setDocID(lemur::api::DOCID_T &docid)
			{
				docID = docid;
			}
			virtual lemur::api::DOCID_T getDocID() const {return docID;}

			virtual void setDistances(std::vector<link::api::DIST_T> &dists)
			{
				distances.resize(dists.size());
				distances = dists;
			}
			virtual std::vector<link::api::DIST_T> getDistances() const
			{
				return distances;
			}

			/** 获取link在窗口值为无穷大时出现的次数 */
			virtual link::api::LINKCOUNT_T getLinkCount() const;

			/** 获取link在指定窗口大小下出现的次数 */
			virtual link::api::LINKCOUNT_T getLinkCount(const int &windowSize) const;

		protected:
			link::api::LINKID_T linkID;
			lemur::api::DOCID_T docID;//文档ID
			std::vector<link::api::DIST_T> distances;//link中term之间的距离
		private:

		};
	}
}



#endif //LINK_INFO_HPP