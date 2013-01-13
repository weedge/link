/*
  author: wy
 */

#include "common_headers.hpp"
//#include "IndexManager.hpp"
#include "KeyfileLinkIndex.hpp"
using namespace lemur::api;
using namespace link::api;

int main(int argc, char *argv[]) {
  KeyfileLinkIndex *ind;
  if (argc < 3) {
    cerr << "usage: dumpLink <index_name> <internal/external linkid> [-ext]" 
	 << endl;
    exit (1);
  }

  const char *indexTOCFile = argv[1];
  int len = strlen(indexTOCFile);
  if (!(len>4 && indexTOCFile[len-9]=='.')) {
	  ; // it must have format .xxxx_xxx 
	  std::cerr<<"Index file must have format .xxxx_xxx"<<std::endl;
  }

  const char *extension = &(indexTOCFile[len-8]);
  if ((!strcmp(extension, "link_key")) ||
	  (!strcmp(extension, "LINK_KEY"))) {
		  ind = new link::api::KeyfileLinkIndex();
  } else {
	  std::cerr<<"unknown index file extension"<<std::endl;
  }
  if (!(ind->open(indexTOCFile))) {
	  std::cerr<<"open linkindex failed"<<std::endl;
  }
  
  LINKID_T did;
  if (argc == 3)
    did = atoi(argv[2]);
  else did = ind->linkTerm(argv[2]);
  
  cout << ind->linkTerm(did) << endl;
  DocInfoList *lList = ind->InvlinkDocList(did);
  if (lList == NULL) {
    cerr << ": empty docInfoList" << endl;
    exit (1);
  }
  
  vector<int> vecWinSize = ind->winSizes();
  int wincount  = ind->winCount();
  for (COUNT_T i = 0; i < wincount; i++)
	  cout << vecWinSize[i] << " ";
  InvLinkDocInfo *info;
  lList->startIteration();
  while (lList->hasMore()) {
    info = dynamic_cast<InvLinkDocInfo *>(lList->nextEntry());
	const lemur::api::LOC_T *lc = info->linkFreq();
    //cout << ind->document(info->docID()) << "/t" ;
    if (lc != NULL) {
      for (COUNT_T i = 0; i < wincount; i++)
		cout << lc[i] << " ";
    }
    cout << endl;
  }
  delete lList;
  delete(ind);
  return 0;
}
