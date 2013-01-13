#include "BasicLinkDocStream.hpp"
#include "Link.hpp"
using namespace link::parse;


void BasicLinkDoc::startTermIteration() const
{
	// ensure the start position of the links
	docStr->seekg(startPos);
	curWord = buf1;
	// peek one link
	*docStr >> curWord;
}

void BasicLinkDoc::skipToEnd() const
{
	startTermIteration();
	while (hasMore()) {
		nextLink();
	}
}
const link::api::Link * BasicLinkDoc::nextLink() const
{
	li.setLinkStr(curWord);
	li.linkToPair(curWord);
	if (curWord == buf1) {
		curWord = buf2;
	} else {
		curWord = buf1;
	}
	*docStr >> curWord;
	return &li;
}
void BasicLinkDoc::readID()
{
	// get id
	*docStr >> id;
	int len= strlen(id);
	if (id[len-1]!='>') {
		throw lemur::api::Exception("BasicTokenDoc","ill-formatted doc id, > expected");
	}
	id[len-1]='\0';
	startPos = docStr->tellg(); // record the start position of terms
}


BasicLinkDocStream::BasicLinkDocStream (const string &inputFile)
{
	strcpy(file, inputFile.c_str());
	ifs = new ifstream(file, ios::in);
	if (ifs->fail() ) {
		throw lemur::api::Exception("BasicDocStream", "can't open BasicDocStream source file");
	}
}


bool BasicLinkDocStream::hasMore()
{
	bool moreDoc = false;
	if (!nextTokenRead) {
		moreDoc = *ifs >> buf;
		nextTokenRead = true;
		if (moreDoc && strcmp(buf, "<DOC")) {
			cerr << " actual token seen: "<< buf << endl;
			throw lemur::api::Exception("BasicDocStream", "begin doc marker expected");
		}
	}

	return moreDoc; 
}


void BasicLinkDocStream::startDocIteration()
{
	ifs->close();
	ifs->open(file);
	ifs->seekg(0);
	ifs->clear(); 
	nextTokenRead =false;
}

lemur::api::Document* BasicLinkDocStream::nextDoc()
{
	// fails to initialize properly, preventing reuse of a 
	// BasicDocStream (or opening more than one).
	// static BasicTokenDoc doc(ifs);
	// make it an instance attribute
	//  static BasicTokenDoc doc;
	doc.docStr = ifs;
	doc.readID();
	nextTokenRead = false;
	return &doc;
}