// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include "filesys.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{
    readerCount = 0;
    userCount = 0;
    rclock = new Lock("readCount Lock");
    rwlock = new Lock("reader-writer Lock");
    printf("lockname: %s\n", rwlock->getName());

    time_t currentTime = time(NULL);
    createTime = currentTime;
    lastAccessTime = currentTime;
    lastWriteTime = currentTime;
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors + numSectors / NumDirect) //nextHdr也需要sector！
	return FALSE;		// not enough space

    FileHeader *hdr = this;
    FileHeader *nextHdr;
    int curSec;     //用于帮助第一个之后的文件头writeback
    int i;
    for (i = 0; i < numSectors; i++){
        if(i % NumDirect == 0 && i != 0){   //每个文件头里放k * (NumDirect - 1)个索引
            nextHdr = new FileHeader;
            hdr->nextHdrSector = freeMap->Find();
            // printf("allocate %d\n", hdr->nextHdrSector);
            if(i != NumDirect)  hdr->WriteBack(curSec);     //第一个文件头不需要也无法在这里writeback
            curSec = hdr->nextHdrSector;
            hdr = nextHdr;
        }
        hdr->dataSectors[i % NumDirect] = freeMap->Find();
        // printf("allocate %d\n", hdr->dataSectors[i % NumDirect]);
    }
    hdr->nextHdrSector = -1;    //最后一个文件头
	if(i >= NumDirect)  hdr->WriteBack(curSec);     //最后一个文件头writeBack回磁盘
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    FileHeader *hdr = this;
    FileHeader *nextHdr;
    for (int i = 0; i < numSectors; i++) {
        if(i % NumDirect == 0 && i != 0){
            nextHdr = new FileHeader;
            nextHdr->FetchFrom(hdr->nextHdrSector);
            hdr = nextHdr;
        }
        ASSERT(freeMap->Test((int) hdr->dataSectors[i % NumDirect]));  // ought to be marked!
        freeMap->Clear((int) hdr->dataSectors[i % NumDirect]);
    }
    
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    // printf("FileHeader writing back to %d\n", sector);
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int tarSector = offset / SectorSize;
    int noHdr = tarSector / NumDirect;
    int tarIndex = tarSector % NumDirect;
    FileHeader *hdr = this;
    FileHeader *nextHdr;
    for(int i = 0; i < noHdr; i ++){
        nextHdr = new FileHeader;
        nextHdr->FetchFrom(hdr->nextHdrSector);
        hdr = nextHdr;
    }
    return(hdr->dataSectors[tarIndex]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    // FileHeader *hdr = this;
    // FileHeader *nextHdr;
    // int totalBytes = numBytes;
    // while(hdr->nextHdrSector != -1){
    //     totalBytes += hdr->numBytes;
    //     nextHdr = new FileHeader;
    //     nextHdr->FetchFrom(hdr->nextHdrSector);
    //     hdr = nextHdr;
    // }
    // return totalBytes;
    // printf("total: %d, numBytes: %d", totalBytes, numBytes);
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    FileHeader* hdr = this;
    for (i = 0; i < numSectors; i++){
        if (i !=0 && i % NumDirect ==0 ){ 
            FileHeader* tmp_hdr = new FileHeader;
            tmp_hdr->FetchFrom(hdr->nextHdrSector);
            hdr = tmp_hdr;
        }
        printf("%d ", hdr->dataSectors[i%NumDirect]);
    }
	
    printf("\nCreate Time: %ld Last Access Time: %ld Last Write Time: %ld \n",createTime,lastAccessTime,lastWriteTime);
    printf("\nFile contents:\n");
    hdr = this;
    for (i = k = 0; i < numSectors; i++) {
        if (i !=0 && i % NumDirect ==0 ){ 
            FileHeader* tmp_hdr = new FileHeader;
            tmp_hdr->FetchFrom(hdr->nextHdrSector);
            hdr = tmp_hdr;
        }
	    synchDisk->ReadSector(hdr->dataSectors[i%NumDirect], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    // printf("before delete\n");
    delete [] data;
    // printf("after delete\n");
}

void FileHeader::AddSector(BitMap *freeMap, OpenFile* freeMapFile, int FirstHdrSec){
    time_t currentTime = time(NULL);
    createTime = currentTime;
    lastAccessTime = currentTime;
    lastWriteTime = currentTime;
    
    if (freeMap->NumClear() < 2) //nextHdr需要一个sector
	return ;		// not enough space

    FileHeader *hdr = this;
    FileHeader *nextHdr;
    int curSec = FirstHdrSec;     //用于帮助第一个之后的文件头writeback
    int i;
    for (i = 0; i < numSectors; i++){   //先找到最后一个文件头
        if(i % NumDirect == 0 && i != 0){
            nextHdr = new FileHeader;
            nextHdr->FetchFrom(hdr->nextHdrSector);
            curSec = hdr->nextHdrSector;
            hdr = nextHdr;
        }
        // hdr->dataSectors[i % NumDirect] = freeMap->Find();
        // printf("allocate %d\n", hdr->dataSectors[i % NumDirect]);
    }
    if(i % NumDirect == 0 && i != 0){     //当前最后一个文件头刚好满了，需要新增一个文件头 
        int nextHdrSec = freeMap->Find();
        hdr->nextHdrSector = nextHdrSec;    //先把当前最后文件头的下一文件头sec从-1改成新值
        hdr->WriteBack(curSec);     //把当前最后文件头写回
        nextHdr = new FileHeader;
        hdr = nextHdr;
        hdr->nextHdrSector = -1;    //新的最后文件头的下一文件头sec为-1
        curSec = nextHdrSec;    //新的最后文件头所在sec就是刚刚分配的
        printf("Added a new fileHeader on sector: %d\n", curSec);
    }
    hdr->dataSectors[i % NumDirect] = freeMap->Find();
    printf("Added a new Sector: %d\n", hdr->dataSectors[i % NumDirect]);
	hdr->WriteBack(curSec);     //最后一个文件头writeBack回磁盘
    freeMap->WriteBack(freeMapFile);
    numBytes += SectorSize;
    numSectors  += 1;
    return;
    
}