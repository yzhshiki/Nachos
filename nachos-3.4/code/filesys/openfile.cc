// openfile.cc 
//	Routines to manage an open Nachos file.  As in UNIX, a
//	file must be open before we can read or write to it.
//	Once we're all done, we can close it (in Nachos, by deleting
//	the OpenFile data structure).
//
//	Also as in UNIX, for convenience, we keep the file header in
//	memory while the file is open.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "filehdr.h"
#include "openfile.h"
#include "system.h"
#ifdef HOST_SPARC
#include <strings.h>
#endif

//----------------------------------------------------------------------
// OpenFile::OpenFile
// 	Open a Nachos file for reading and writing.  Bring the file header
//	into memory while the file is open.
//
//	"sector" -- the location on disk of the file header for this file
//----------------------------------------------------------------------

OpenFile::OpenFile(int sector)
{ 
    hdr = new FileHeader;
    hdrSector = sector;
    hdr->FetchFrom(hdrSector);
    // printf("lock: %d\n", hdr->rwlock->getName());
    hdr->rwlock->Acquire();
    // printf("hdr fetch\n");
    if(hdrSector > 1)
        hdr->userCount ++;
    // printf("%s OpenFile sector: %d, userCount: %d\n", currentThread->getName(), sector, hdr->userCount);
    hdr->WriteBack(hdrSector);
    hdr->rwlock->Release();
    seekPosition = 0;
}

//----------------------------------------------------------------------
// OpenFile::~OpenFile
// 	Close a Nachos file, de-allocating any in-memory data structures.
//----------------------------------------------------------------------

OpenFile::~OpenFile()
{
    if(hdrSector> 1){
        // printf("DeletingFile   Sector: %d userCount: %d\n", hdrSector, hdr->userCount);
        hdr->rwlock->Acquire();
        hdr->FetchFrom(hdrSector);
        
        hdr->userCount --;
        // printf("%s Deleted File on Sector: %d userCount: %d\n", currentThread->getName(), hdrSector, hdr->userCount);
        hdr->WriteBack(hdrSector);
        hdr->rwlock->Release();

        // hdr->FetchFrom(hdrSector);
        // printf("%s Deleted File on Sector: %d userCount: %d\n", currentThread->getName(), hdrSector, hdr->userCount);
        delete hdr;
        
    }
}

//----------------------------------------------------------------------
// OpenFile::Seek
// 	Change the current location within the open file -- the point at
//	which the next Read or Write will start from.
//
//	"position" -- the location within the file for the next Read/Write
//----------------------------------------------------------------------

void
OpenFile::Seek(int position)
{
    seekPosition = position;
}	

//----------------------------------------------------------------------
// OpenFile::Read/Write
// 	Read/write a portion of a file, starting from seekPosition.
//	Return the number of bytes actually written or read, and as a
//	side effect, increment the current position within the file.
//
//	Implemented using the more primitive ReadAt/WriteAt.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//----------------------------------------------------------------------

int
OpenFile::Read(char *into, int numBytes)
{
    // printf("%s Asking rclock\n", currentThread->getName());
    hdr->rclock->Acquire();
    hdr->FetchFrom(hdrSector);
    hdr->readerCount ++;
    hdr->WriteBack(hdrSector);
    if(hdr->readerCount == 1){
        // printf("%s Asking rwlock\n", currentThread->getName());
        hdr->rwlock->Acquire();
        // printf("reading rwlock: %d", hdr->rwlock);
    }   
    hdr->rclock->Release();

    hdr->FetchFrom(hdrSector);
    int result = ReadAt(into, numBytes, seekPosition);
    time_t currentTime = time(NULL);
    hdr->lastAccessTime = currentTime; 
    // currentThread->Yield();
    seekPosition += result;
    hdr->WriteBack(hdrSector);

    hdr->rclock->Acquire();
    hdr->FetchFrom(hdrSector);
    hdr->readerCount --;
    hdr->WriteBack(hdrSector);
    if(hdr->readerCount == 0)
        hdr->rwlock->Release();
    hdr->rclock->Release();

    return result;
}

int
OpenFile::Write(char *into, int numBytes)
{
    hdr->rwlock->Acquire();
    // printf("writing rwlock: %d", hdr->rwlock);
    hdr->FetchFrom(hdrSector);
    int result = WriteAt(into, numBytes, seekPosition);
    if(result == 0)
        printf("Writing action Exceed!\n");
    // currentThread->Yield(); //for test
    seekPosition += result;
    time_t currentTime = time(NULL);
    hdr->lastAccessTime = currentTime; 
    hdr->lastWriteTime = currentTime;
    hdr->WriteBack(hdrSector);
    // printf("Writed %d\n", result);

    hdr->WriteBack(hdrSector);
    hdr->rwlock->Release();
    return result;
}

//----------------------------------------------------------------------
// OpenFile::ReadAt/WriteAt
// 	Read/write a portion of a file, starting at "position".
//	Return the number of bytes actually written or read, but has
//	no side effects (except that Write modifies the file, of course).
//
//	There is no guarantee the request starts or ends on an even disk sector
//	boundary; however the disk only knows how to read/write a whole disk
//	sector at a time.  Thus:
//
//	For ReadAt:
//	   We read in all of the full or partial sectors that are part of the
//	   request, but we only copy the part we are interested in.
//	For WriteAt:
//	   We must first read in any sectors that will be partially written,
//	   so that we don't overwrite the unmodified portion.  We then copy
//	   in the data that will be modified, and write back all the full
//	   or partial sectors that are part of the request.
//
//	"into" -- the buffer to contain the data to be read from disk 
//	"from" -- the buffer containing the data to be written to disk 
//	"numBytes" -- the number of bytes to transfer
//	"position" -- the offset within the file of the first byte to be
//			read/written
//----------------------------------------------------------------------

int
OpenFile::ReadAt(char *into, int numBytes, int position)
{
    int fileLength = hdr->FileLength();
    int i, firstSector, lastSector, numSectors;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength))
    	return 0; 				// check request
    if ((position + numBytes) > fileLength)		
	    numBytes = fileLength - position;
    DEBUG('f', "Reading %d bytes at %d, from file of length %d.\n", 	
			numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    // read in all the full and partial sectors that we need
    buf = new char[numSectors * SectorSize];
    for (i = firstSector; i <= lastSector; i++)	
        synchDisk->ReadSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);

    // copy the part we want
    bcopy(&buf[position - (firstSector * SectorSize)], into, numBytes);
    delete [] buf;
    return numBytes;
}

int
OpenFile::WriteAt(char *from, int numBytes, int position)
{
    // printf("position: %d", position);
    int fileLength = hdr->FileLength();
    // printf("numBytes: %d, position: %d, filelen: %d\n", numBytes, position, fileLength);
    int i, firstSector, lastSector, numSectors;
    bool firstAligned, lastAligned;
    char *buf;

    if ((numBytes <= 0) || (position >= fileLength)){
        // ASSERT(false);
        fileSystem->AddSector(hdr, hdrSector);
        return WriteAt(from, numBytes, position);
    }
	
    if ((position + numBytes) > fileLength)
	numBytes = fileLength - position;
    DEBUG('f', "Writing %d bytes at %d, from file of length %d.\n", 	
			numBytes, position, fileLength);

    firstSector = divRoundDown(position, SectorSize);
    lastSector = divRoundDown(position + numBytes - 1, SectorSize);
    numSectors = 1 + lastSector - firstSector;

    buf = new char[numSectors * SectorSize];

    firstAligned = (position == (firstSector * SectorSize));
    lastAligned = ((position + numBytes) == ((lastSector + 1) * SectorSize));

// read in first and last sector, if they are to be partially modified
    if (!firstAligned)
        ReadAt(buf, SectorSize, firstSector * SectorSize);	
    if (!lastAligned && ((firstSector != lastSector) || firstAligned))
        ReadAt(&buf[(lastSector - firstSector) * SectorSize], 
				SectorSize, lastSector * SectorSize);	

// copy in the bytes we want to change 
    bcopy(from, &buf[position - (firstSector * SectorSize)], numBytes);

// write modified sectors back
    for (i = firstSector; i <= lastSector; i++)	
        synchDisk->WriteSector(hdr->ByteToSector(i * SectorSize), 
					&buf[(i - firstSector) * SectorSize]);
    delete [] buf;
    return numBytes;
}

//----------------------------------------------------------------------
// OpenFile::Length
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
OpenFile::Length() 
{ 
    return hdr->FileLength(); 
}
