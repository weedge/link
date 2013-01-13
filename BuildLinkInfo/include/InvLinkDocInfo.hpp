/*======================================================================
 * NAME DATE - COMMENTS
 * wy 12/2011 - created 根据具体连接结构进行修改。for invLinkDocList.
 *========================================================================*/
#ifndef _INVLINKDOCINFO_HPP
#define _INVLINKDOCINFO_HPP

#include "LinkTypes.hpp"

namespace link 
{
  namespace api 
  {

    class InvLinkDocInfo: public lemur::api::DocInfo 
	{
    public: 
      InvLinkDocInfo() {linkcount=NULL;}
      ~InvLinkDocInfo() {} 
  
      const lemur::api::LOC_T* linkFreq() const {return linkcount; }
      void linkFreq(const lemur::api::LOC_T* lc) {linkcount = lc;}

    private:
		const lemur::api::LOC_T* linkcount;  // list of linkcounts in this doc (size is count)
    };
  }
}

#endif
