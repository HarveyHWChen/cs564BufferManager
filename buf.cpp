#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include "page.h"
#include "buf.h"

#define ASSERT(c)  { if (!(c)) { \
		       cerr << "At line " << __LINE__ << ":" << endl << "  "; \
                       cerr << "This condition should hold: " #c << endl; \
                       exit(1); \
		     } \
                   }

//----------------------------------------
// Constructor of the class BufMgr
//----------------------------------------

BufMgr::BufMgr(const int bufs)
{
    numBufs = bufs;

    bufTable = new BufDesc[bufs];
    memset(bufTable, 0, bufs * sizeof(BufDesc));
    for (int i = 0; i < bufs; i++) 
    {
        bufTable[i].frameNo = i;
        bufTable[i].valid = false;
    }

    bufPool = new Page[bufs];
    memset(bufPool, 0, bufs * sizeof(Page));

    int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
    hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table

    clockHand = bufs - 1;
}

/**
 * Flushes out all dirty pages and deallocates the buffer pool and the BufDesc table.
 */
BufMgr::~BufMgr() {
  // TODO: Implement this method by looking at the description in the writeup.
  for(int i = 0; i < numBufs; i++){
    BufDesc bufEntry = bufTable[i];
    if(bufEntry.dirty){
      // flush this page to disk
      int frameNo = bufEntry.frameNo;
      bufEntry.file->writePage(bufEntry.pageNo, bufPool + frameNo);
    }
  }
  delete[] bufTable;
  delete[] bufPool;
  delete hashTable;
}


/**
 * Allocates a free frame using the clock algorithm; if necessary, writing a dirty page back to disk.
 * @return OK on success
 * @return BUFFEREXCEEDED if all buffer frames are pinned
 * @return UNIXERR if the call to the I/O layer returned an error when a dirty page was being written to disk 
 */
const Status BufMgr::allocBuf(int & frame) {
  // TODO: Implement this method by looking at the description in the writeup.
  
  return OK;
}

/**
 * Read a page. First check if its in a buffer pool
 * @return OK if no errors occurred
 * @return UNIXERR if a Unix error occurred
 * @reutrn BUFFEREXCEEDED if all buffer frames are pinned
 * @return HASHTBLERROR if a hash table error occured
 */	
const Status BufMgr::readPage(File* file, const int PageNo, Page*& page) {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}

/**
 * Decrements the pinCnt of the frame containing (file, PageNo)
 * if dirty == true, sets the dirty bit
 * @return OK if no errors occurred
 * @return HASHNOTFOUND if the page is not in the buffer pool hash table
 * @return PAGENOTPINNED if the pin count is already 0
 */
const Status BufMgr::unPinPage(File* file, const int PageNo, 
			       const bool dirty) {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}

/**
 * Allocate an empty page in the specified file by invoking the file->allocatePage() method;
 * Then allocBuf() is called to obtain a buffer pool frame.
 * An entry is inserted into the hash table and Set() is invoked on the frame to set it up properly
 * @return OK if no errors occurred
 * @return UNIXERR if a Unix error occurred
 * @return BUFFEREXCEEDED if all buffer frames are pinned
 * @return HASHTBLERROR if a hash table error occurred
 */
const Status BufMgr::allocPage(File* file, int& pageNo, Page*& page)  {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}

/**
 * If a page exists in the buffer pool, clear the page, remove the corresponding entry from the hash table and dispose the page in the file as well. 
 * @return the status of the call to dispose the page in the file.
 */
const Status BufMgr::disposePage(File* file, const int pageNo) {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}

/**
 * Scan bufTable for pages belonging to the file, for every page:
 * 1. if the page is dirty, call file->writePage() to flush the page to disk and then set the dirty bit for the page to false;
 * 2. remove the page from the hashtable (whether the page is clean or dirty);
 * 3. invoke the Clear() method on the page frame.
 * @return OK if no errors occurred
 * @return PAGEPINNED if some page of the file is pinned
 */
const Status BufMgr::flushFile(const File* file) {
	// TODO: Implement this method by looking at the description in the writeup.
	return OK;
}


void BufMgr::printSelf(void) 
{
    BufDesc* tmpbuf;
  
    cout << endl << "Print buffer...\n";
    for (int i=0; i<numBufs; i++) {
        tmpbuf = &(bufTable[i]);
        cout << i << "\t" << (char*)(&bufPool[i]) 
             << "\tpinCnt: " << tmpbuf->pinCnt;
    
        if (tmpbuf->valid == true)
            cout << "\tvalid\n";
        cout << endl;
    };
}


