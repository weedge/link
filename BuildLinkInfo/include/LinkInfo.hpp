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
		 * ����link���ĵ��г��ֵĴ������������Ϣ
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

			/** ��ȡlink�ڴ���ֵΪ�����ʱ���ֵĴ��� */
			virtual link::api::LINKCOUNT_T getLinkCount() const;

			/** ��ȡlink��ָ�����ڴ�С�³��ֵĴ��� */
			virtual link::api::LINKCOUNT_T getLinkCount(const int &windowSize) const;

		protected:
			link::api::LINKID_T linkID;
			lemur::api::DOCID_T docID;//�ĵ�ID
			std::vector<link::api::DIST_T> distances;//link��term֮��ľ���
		private:

		};
	}
}



#endif //LINK_INFO_HPP