/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 03/2011 - created
 *========================================================================*/
#ifndef LINK_DOCLISTSEGMENTREADER_HPP
#define LINK_DOCLISTSEGMENTREADER_HPP

#include "File.hpp"
#include "InvLinkDocList.hpp"
#include "ReadBuffer.hpp"
#include <string>
#include <vector>


namespace link{
	namespace file{
		class LinkDocListSegmentReader{
		public:
			LinkDocListSegmentReader();
			LinkDocListSegmentReader(int segment, std::string &baseName, 
									lemur::file::File* stream, int readBufferSize, const std::vector<int> &winsizes);

			~LinkDocListSegmentReader();

			link::api::InvLinkDocList* next();

			bool operator<(const LinkDocListSegmentReader &other)const;

			void pop(){_top = next();}

			/// Return the current DocInfoList in the segment  for only-read in the function operator <.
			const link::api::InvLinkDocList* top() const{return _top;}

			link::api::InvLinkDocList* top(){return _top;}

			lemur::file::File* getFile(){return _stream;}

			int getSegment() const{return _segment;}

			std::string getName() const{return _name;}


		private:
			int _segment;//segment number
			lemur::file::ReadBuffer* _file;//read buffer
			lemur::file::File* _stream;//file stream
			link::api::InvLinkDocList* _top;//the top of the InvLinkDocList queue
			std::string _name;//index name
			std::vector<int> _winSizes;//window sizes

		};
	}
}









#endif //LINK_DOCLISTSEGMENTREADER_HPP