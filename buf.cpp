/**
  Program Title: Buffer Manager

  File Purpose: The heart of the buffer manager. Implement clock algorithm, reading page,
                unpin page, allocate page, dispose page and flush file. 

  Semester: CS564 Fall 2014
  
  Authors & ID: Hanwen Chen(9069978907);
                Qiaoya Cui (9070006128);

  Lecture's Name: Jeff Naughton
 */

#include <memory.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <iostream>
#include <stdio.h>
#include "page.h"
#include "buf.h"
#include <vector>

#define ASSERT(c)  { if (!(c)) { \
		       cerr << "At line " << __LINE__ << ":" << endl << "  "; \
                       cerr << "This condition should hold: " #c << endl; \
                       exit(1); \
		     } \
                   }

#define CHKSTAT(c) { if(c != OK) { \
                          return c; \
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
BufMgr::~BufMgr() 
{
  // flush pages inside the buffer pool if necessary
  for(int i = 0; i < numBufs; i++){
    BufDesc* frame = &bufTable[i];
    if(frame->dirty){
      // flush this page to disk
      int frameNo = frame->frameNo;
      frame->file->writePage(frame->pageNo, bufPool + frameNo);
    }
  }
  // clean the allocated memory
  delete[] bufTable;
  delete[] bufPool;
  delete hashTable;
}


/**
 * Allocates a free frame using the clock algorithm; if necessary, writing a dirty page back to disk.
 * @param frame the addres where index of frame to be allocated has stored
 * @return OK on success
 * @return BUFFEREXCEEDED if all buffer frames are pinned
 * @return UNIXERR if the call to the I/O layer returned an error when a dirty page was being written to disk 
 */
const Status BufMgr::allocBuf(int & frame) 
{
  // first check if all pinned
  bool unpinned = false;
  for(int i = 0; i < numBufs; i++){
    if(bufTable[i].pinCnt <= 0){
      // found an unpinned frame
      unpinned = true;
      break;
    }
  }
  if(unpinned == false) return BUFFEREXCEEDED;
  //unsigned int startHand = clockHand;
  while(true){
    advanceClock();
    BufDesc* frameInfo = &bufTable[clockHand];
    if(frameInfo->valid){
      if(frameInfo->refbit){
	frameInfo->refbit = false;
	continue;
      } else {
	if(frameInfo->pinCnt > 0){
	  continue;
	} else {
	  if(frameInfo->dirty){
	    // flush page to disk
	    Status s = frameInfo->file->writePage(frameInfo->pageNo, bufPool + frameInfo->frameNo);
	    CHKSTAT(s); // UNIXERR
	  }
	  break;
	}
      }
    } else {
      break;
    }
  }
  // set frame
  if(bufTable[clockHand].valid){
    hashTable->remove(bufTable[clockHand].file, bufTable[clockHand].pageNo);
  }
  bufTable[clockHand].Clear();
  frame = bufTable[clockHand].frameNo;
  return OK;
}

/**
 * Read a page. First check if its in a buffer pool
 * @param file the pointer to the file
 * @param PageNo the index of page inside the file
 * @param page the reference of the pointer pointing to the address where page to be stored
 * @return OK if no errors occurred
 * @return UNIXERR if a Unix error occurred
 * @reutrn BUFFEREXCEEDED if all buffer frames are pinned
 * @return HASHTBLERROR if a hash table error occured
 */	
const Status BufMgr::readPage(File* file, const int PageNo, Page*& page) 
{
  int frameNo = -1;  
  Status s = hashTable->lookup(file, PageNo, frameNo);
  if(s == OK){
    // it's in the buffer pool
    BufDesc* frame = &bufTable[frameNo];
    frame->refbit = true;
    frame->pinCnt++;
    page = &(bufPool[frame->frameNo]);
  } else {
    // it's not in the buffer pool
    s = allocBuf(frameNo);
    CHKSTAT(s); // BUFFEREXCEEDED, UNIXERR
    Page* pPage = &bufPool[frameNo];
    s = file->readPage(PageNo, pPage);
    //if(statues != OK) return statues;
    CHKSTAT(s); // UNIXERR
    s = hashTable->insert(file, PageNo, frameNo);
    CHKSTAT(s); // HASHTBLERR
    bufTable[frameNo].Set(file, PageNo);
    page = &(bufPool[frameNo]);
  }
  return OK;
}

/**
 * Decrements the pinCnt of the frame containing (file, PageNo)
 * if dirty == true, sets the dirty bit
 * @param file the pointer to the file
 * @param PageNo the index of the page inside the file
 * @param dirty the dirty bit indicates if the page has been updated
 * @return OK if no errors occurred
 * @return HASHNOTFOUND if the page is not in the buffer pool hash table
 * @return PAGENOTPINNED if the pin count is already 0
 */
const Status BufMgr::unPinPage(File* file, const int PageNo, 
			       const bool dirty) 
{
  int frameNo = -1;
  Status s = hashTable->lookup(file, PageNo, frameNo);
  CHKSTAT(s); // HASHNOTFOUND
  BufDesc* frame = &bufTable[frameNo];
  if(dirty){
    frame->dirty = true;
  }
  if(frame->pinCnt <= 0){
    return PAGENOTPINNED;
  } else {
    frame->pinCnt--;
    if(frame->pinCnt < 0) frame->pinCnt = 0;
  }
  return OK;
}

/**
 * Allocate an empty page in the specified file by invoking the file->allocatePage() method;
 * Then allocBuf() is called to obtain a buffer pool frame.
 * An entry is inserted into the hash table and Set() is invoked on the frame to set it up properly
 * @param file the pointer to the file
 * @param pageNo the index of the page inside the file
 * @param page the reference of the pointer pointing to the address where page to be stored
 * @return OK if no errors occurred
 * @return UNIXERR if a Unix error occurred
 * @return BUFFEREXCEEDED if all buffer frames are pinned
 * @return HASHTBLERROR if a hash table error occurred
 */
const Status BufMgr::allocPage(File* file, int& pageNo, Page*& page)  
{
  Status s = file->allocatePage(pageNo);
  CHKSTAT(s); // UNIXERR
  s = readPage(file, pageNo, page);
  CHKSTAT(s); // UNIXERR, BUFFEREXCEEDED, HASHTBLERR
  return OK;
}

/**
 * If a page exists in the buffer pool, clear the page, remove the corresponding entry from the hash table and dispose the page in the file as well. 
 * @param file the pointer to the file
 * @param pageNo the index of the page inside the file
 * @return the status of the call to dispose the page in the file.
 */
const Status BufMgr::disposePage(File* file, const int pageNo) 
{
  int frameNo = -1;
  Status s = hashTable->lookup(file, pageNo, frameNo);
  if(s == OK){
    BufDesc* frame = &bufTable[frameNo];
    frame->Clear();
    hashTable->remove(file, pageNo);
  }
  s = file->disposePage(pageNo);
  return s;
}

/**
 * Scan bufTable for pages belonging to the file, for every page:
 * 1. if the page is dirty, call file->writePage() to flush the page to disk and then set the dirty bit for the page to false;
 * 2. remove the page from the hashtable (whether the page is clean or dirty);
 * 3. invoke the Clear() method on the page frame.
 * @param file the pointer to the file
 * @return OK if no errors occurred
 * @return PAGEPINNED if some page of the file is pinned
 */
const Status BufMgr::flushFile(const File* file) 
{
  // first check if all pages of this file are unpinned
  File* pFile = const_cast<File*>(file);
  std::vector<BufDesc*> frames;
  for(int i = 0; i < numBufs; i++){
    BufDesc* frame = &bufTable[i];
    if(frame->file == pFile){
      frames.push_back(frame);
    }
    if(frame->pinCnt > 0) return PAGEPINNED;
  }
  for(unsigned int i = 0; i < frames.size(); i++){
    BufDesc* pFrame = frames[i];
    if(pFrame->dirty){
      // flush to disk
      Status s = pFile->writePage(pFrame->pageNo, bufPool + pFrame->frameNo);
      CHKSTAT(s);
      pFrame->dirty = false;
    }
    Status s = hashTable->remove(pFile, pFrame->pageNo);
    CHKSTAT(s);
    pFrame->Clear();
  }
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


