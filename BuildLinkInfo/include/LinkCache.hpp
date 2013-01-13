/**======================================
/*
 * NAME DATE - COMMENTS
 * wy 02/2011 - created 
 *========================================================================*/
#ifndef _LINK_TERMCACHE_HPP
#define _LINK_TERMCACHE_HPP

#include "LinkTypes.hpp"

namespace link 
{
  namespace utility 
  {
    
	#define LINKCACHE_MAX_LINK_LENGTH 12 //大数据调试的时候需要改
	#define LINKCACHE_SIZE    (128 * 1024 * 1024)//128M
	#define LINKCACHE_BUCKETS (LINKCACHE_SIZE/24)
    ///External to internal link id hash table for optimizing indexing with KeyfileLinkIndex.
    class LinkCache 
	{
    public:
      LinkCache();
	  /// add a link<termID, termID> "lefttermID-righttermID"to the cache.
	  bool add( const char* linkName, link::api::LINKID_T LinkID );
      /// lookup a link in the cache. Returns internal LinkID or -1 if not found.
	  link::api::LINKID_T find( const char* linkName ) const;
      /// clear the cache.
      void clear();

    private:
		link::api::LINKID_T _hashFunction( const char* linkName ) const;

      struct cache_entry //24Byte
	  {
        char link_str[LINKCACHE_MAX_LINK_LENGTH];
		link::api::LINKID_T linkID;
      };

      struct cache_entry _linkCache[ LINKCACHE_SIZE / sizeof (struct cache_entry) ];
    };
  }
}

#endif // _LINK_TERMCACHE_HPP