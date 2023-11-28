// QTAtomHandler.cpp by Simon Clarke (S.J.Clarke@herts.ac.uk) - Copyright 1996-8

#include <string.h>
#include <stdarg.h>
#include <ByteOrder.h>
#include <List.h>
#include <File.h>
#include <MediaDefs.h>
#include <zlib.h>

#include "QTAtomHandler.h"
#include "QTTrack.h"


namespace BPrivate {
	extern bool media_debug;
}

QTAtomHandler::QTAtomHandler()
{
	fTrackList = new BList();
	fCurrentTrack = NULL;

	fTempBuffer = 0;
	fMDIACurrent = false;

	fBufferSize = 0;

	fMoovSize = 0; fStoredMDATSize = 0;	 
	fMdatFound = false;

	fAudioTrackCount = fVideoTrackCount = 0;
	fHeaderBuffer = fHeaderPosition = 0;
	
	fDecompressedBuffer = 0;
	fDecompressedPosition = 0;
	
	fCopyright = NULL;
}

QTAtomHandler::~QTAtomHandler()
{
	if (fCopyright)
		free(fCopyright);
	fCopyright = NULL;
	
	if (fTempBuffer)
		free(fTempBuffer);
	if (fHeaderBuffer)
		free(fHeaderBuffer);
	fTempBuffer = fHeaderBuffer = NULL;
		
	if (fTrackList) {
		for (int32 i=0; i < fTrackList->CountItems(); i++) {
			QTTrack *ptr = (QTTrack *)fTrackList->ItemAt(i);
			if (ptr) {
				delete ptr;
			}
		}
		delete fTrackList;
		fTrackList = NULL;
	}
}

void QTAtomHandler::SetTo(BPositionIO *stream)
{
	inStream = stream;
}

const char *QTAtomHandler::Copyright(void)
{
	return fCopyright;
}


status_t QTAtomHandler::IsQuickTime(const entry_ref &file)
{
	/* identify incoming QuickTime file */

	BFile		myFile(&file, B_READ_ONLY);	
	uint32		atoms[2];
	uint32		atomSize, atomCode;

	if (myFile.Read(atoms, 8) != 8)
		return B_ERROR;
			
	atomSize = B_BENDIAN_TO_HOST_INT32(atoms[0]);
	atomCode = B_BENDIAN_TO_HOST_INT32(atoms[1]);

	if (atomSize == 0) /* weirdness going on */
		return B_ERROR;

	switch (atomCode) {
		case QT_mdat:
		case QT_moov: {
			return B_OK;
			break;
		}
	}
	
	/* not quicktime */
	return B_ERROR;
}

BList *QTAtomHandler::TrackList()
{
	return fTrackList;
}

bigtime_t QTAtomHandler::MaxDuration()
{
	double d;

	d = (double)fMVHDHeader.duration / (double)fMVHDHeader.timescale;
	d *= 1000000.0;  /* convert to usecs */

	return (bigtime_t)d;
}

status_t QTAtomHandler::Begin()
{
	if (ParseHeader() == QT_ERROR) 
		return QT_ERROR;
	
	if (ParseAtoms(fMoovSize) == QT_ERROR) {
		DEBUGTEXT("ParseChunks() failed\n");
		return QT_ERROR;
	}
	
//	if (!fMdatFound) {
//		DEBUGTEXT("fMdatFound || fMoovHeaderFound failed!\n");
//		return QT_ERROR;
//	}
	
	_QTdebugprint("QT: Final Image Details TS %ld True Dur %ld Duration %f\n",fMVHDHeader.timescale,fMVHDHeader.duration,(float)fMVHDHeader.duration / (float)fMVHDHeader.timescale);
	
	return QT_OK;	
}

qt_mvhd_details *QTAtomHandler::MVHDHeader()
{
	return &fMVHDHeader;
}

qt_hdlr_details *QTAtomHandler::HDLRHeader()
{
	return &fHDLRHeader;
}

status_t QTAtomHandler::ReadHeader(size_t length)
{
	size_t			readBytes;

	fMoovSize = length;

	fHeaderBuffer = (uint8 *)malloc(length);
	if (!fHeaderBuffer)
		return B_NO_MEMORY;
		
	readBytes = inStream->Read(fHeaderBuffer, length);
	if (readBytes != length)
		return B_ERROR;

	fHeaderPosition = fHeaderBuffer;
	
	return B_OK;
}

status_t QTAtomHandler::ParseHeader()
{
	uint32 			length;
	int32			chunkid;
	uint32			atoms[2];

	inStream->Seek(0,SEEK_SET);
	inStream->Read(atoms, 8);
	
	length = B_BENDIAN_TO_HOST_INT32(atoms[0]);
	chunkid = B_BENDIAN_TO_HOST_INT32(atoms[1]);

	if (chunkid == QT_wide) {
		inStream->Read(atoms, 8);
		length = B_BENDIAN_TO_HOST_INT32(atoms[0]);
		chunkid = B_BENDIAN_TO_HOST_INT32(atoms[1]);
	}
		
	_QTdebugprint("QT: First chunk %ld %.4s\n",length, &chunkid);

	switch (chunkid) {
		case QT_mdat:
			if (!length)
				return B_ERROR;			
				
			fMdatFound = true;
			fStoredMDATSize = length;

			inStream->Seek(length - 8, SEEK_CUR);
			inStream->Read(atoms, 8);
			
			length = B_BENDIAN_TO_HOST_INT32(atoms[0]);
			chunkid = B_BENDIAN_TO_HOST_INT32(atoms[1]);
			if (chunkid == QT_moov) {
				return ReadHeader(length - 8);
			} else
				return B_ERROR;	
			break;
			
		case QT_moov:
			return ReadHeader(length - 8);
			break;

		default:
			// in some strange cases there is no mdat chunk
			// but the first 4 bytes to point us at where
			// the moov chunk is.  so to make that work we
			// check if the mdat length makes sense and if
			// it does we go there and see if there's a moov
			// chunk.
			//
			off_t file_size;
			file_size = inStream->Seek(0, SEEK_END);

			if (length < file_size) {
				inStream->Seek(length, SEEK_SET);
				if (inStream->Read(atoms, 8) != 8)
					break;
				length = B_BENDIAN_TO_HOST_INT32(atoms[0]);
				chunkid = B_BENDIAN_TO_HOST_INT32(atoms[1]);
				if (chunkid == QT_moov)
					return ReadHeader(length - 8);
			}
			break;
	}

	return B_ERROR;
}

status_t QTAtomHandler::ParseAtoms(size_t fileLength)
{
	uint32 			id = 0;
	uint32 			len;
	status_t		myErr = QT_OK;
	
	while (fileLength > 0) {
		if (fDecompressedBuffer && fDecompressedPosition >= fDecompressedBuffer +
			fDecompressedSize) {
			_QTdebugprint("End of compressed chunk\n");
			// End of decompressed chunk.
			free(fDecompressedBuffer);
			fDecompressedBuffer = 0;
		}

		len = ReadMSB32();
		id = ReadMSB32();

		_QTdebugprint("QT -- ATOM %.4s (0x%x) size %d fileLen %d\n", &id, id,
					  len, fileLength);
						
		if (len < 8) {
			_QTdebugprint("QT: Bad chunk with ID %x Len %ld len < 8 (%d remaining)\n",
					   id,len, fileLength);
			fileLength = 0; // bad chunk
			// not an error?
			break;
		}
		
    	switch (id) {
			case QT_wide:
				fileLength -= 8;
				break;

			case QT_dcom:			// Data compression atom
				fCompressionType = ReadMSB32();
				fileLength -= 12;
				break;
				
			case QT_cmvd: {			// Compressed movie data
				fDecompressedSize = ReadMSB32();
				_QTdebugprint("Compressed atom.  Uncompressed size is %d\n",
					fDecompressedSize);
				if (fCompressionType == 'zlib') {
					fDecompressedBuffer = (uint8*) malloc(fDecompressedSize);
					int zerror = uncompress(fDecompressedBuffer, &fDecompressedSize,
						fHeaderPosition, len - 12);
					if (zerror < Z_OK) {
						switch (zerror) {
						case Z_MEM_ERROR:
							_QTdebugprint("Out of memory\n");
							break;
						case Z_BUF_ERROR:
							_QTdebugprint("Not enough room in output buffer\n");
							break;
						case Z_DATA_ERROR:
							_QTdebugprint("The compressed data is invalid\n");
							break;
						case Z_STREAM_ERROR:
							_QTdebugprint("stream structure is inconsistent\n");
							break;
						case Z_NEED_DICT:
							_QTdebugprint("need dictionary\n");
							break;
						}

						_QTdebugprint("inflate failed\n");
						free(fDecompressedBuffer);
						fDecompressedBuffer = 0;
					} else {
						fDecompressedPosition = fDecompressedBuffer;
						fileLength -= len;
						fileLength += fDecompressedSize;
						fHeaderPosition += len - 12;
					}
				} else {
					_QTdebugprint("Unknown meta data compression format\n");
					AdvanceStream(len - 12); // Skip unknown data
					fileLength -= len;
				}

				break;
			}
	
    		// Central Movie Atoms
    		case QT_trak:
 		   		// DEBUGCHUNK("QTChunk: QT_trak\n");
 		   		_QTdebugprint("QTChunk: QT_trak (len %d / 0x%x)\n", len, len);
				fileLength -=8;
				break;
				
			case QT_cmov:
			case QT_moov: {
				// step to next atom in movie header
 				fileLength -= 8;
				break;
			}

//---------------------------------- Data Chunks
		
			case QT_mdat: {
				// data info
				DEBUGCHUNK("QTChunk: QT_mdat\n");
								
				fMdatFound = true;
				fStoredMDATSize = len;

				AdvanceStream(len - 8);
				_QTdebugprint("QT m_dat Size of Data %ld\n",len);
				break;				
			}

			case QT_udta: {
				// leaf atom
				_QTdebugprint("QTChunk: QT_udta len = %ld\n",len);
				fileLength -= ParseUserData(len);
				break;
			}
			
	      	case QT_mdia: 
	      		fileLength -= 8; 
	      		fMDIACurrent = true; 
	      		break;
	      	
      		case QT_minf: {
      			fileLength -= 8;
      			fMDIACurrent = false;
      			break;
      		}
      		
      		case QT_stbl:
      		case QT_edts:
				_QTdebugprint("QTChunk: Start of Leaf Atom %.4s\n",&id);
				fileLength -= 8;
    			break;
    		
//--------------------------------- Headers
    		 
     		case QT_mvhd: {
				DEBUGCHUNK("QTChunk: QT_mvhd\n");
				ReadMVHD(&fMVHDHeader);
				fileLength -= len;
    			break;
			}

			case QT_tkhd: {
 		   		DEBUGCHUNK("QTChunk: QT_tkhd\n");
 		   		
 		   		if (!fCurrentTrack) {
 		   			fCurrentTrack = new QTTrack();
 		   		} else {
 		   			// push onto list, parse last track	   		
 		   			if (ParseIndex(fCurrentTrack) == B_OK)
 		   				fTrackList->AddItem(fCurrentTrack);
					else
						delete fCurrentTrack;
 		   			fCurrentTrack = new QTTrack();
 		   		}
 		   		
 		   		ReadTKHD(&fCurrentTrack->qt_tkhdr);
				
				fileLength -= len;				
				
				fCurrentTrack->SetTrackID(fCurrentTrack->qt_tkhdr.trackid);
				fTrackCount++;
				break;
			}

// QuickTime Edit List Atom

			case QT_elst: {
 		   		DEBUGCHUNK("QTChunk: QT_elst\n");
 		   		ReadELST(fCurrentTrack);
				fileLength -= len;				
				break;
			}

			case QT_mdhd: {
 		   		DEBUGCHUNK("-----------------------------------\nQTChunk: QT_mdhd\n");
 		   		ReadMDHD(&fCurrentTrack->qt_mdhdr);
				fileLength-=len;				
				break;
			}

			case QT_hdlr: {
				#ifdef DEBUGONE
				printf("QTChunk: QT_hdlr = %ld\n",len);
				#endif
				
				ReadHDLR(&fHDLRHeader,len);
				fileLength -= len;				
				
				if (fMDIACurrent) {
					switch (fHDLRHeader.subtype) {
						case 'text': fCurrentTrack->SetType(QT_TEXT); break;
					}
				}
				
				break;
			}

			case QT_load: {
				#ifdef DEBUGONE
				printf("QTChunk: QT_load = %ld\n",len);
				#endif
				
				ReadLOAD(fCurrentTrack);
				
				fileLength -= len;
				break;
			}

			case QT_dinf: {
				// leaf atom
				_QTdebugprint("QTChunk: QT_dinf = %ld\n",len);
				fileLength -= 8;
				break;
			}
				
			case 'dref':
				#ifdef DEBUGONE
				printf("QTChunk: QT_dref = %ld\n",len);
				#endif

        		ReadDREF();

				fileLength -= len;
				break;

//---------------------------------- Ignored content atoms
   			
  			case QT_gmhd:
      		case QT_text:
      		case QT_clip:
      		case QT_skip:
      		case 'free':
      			_QTdebugprint("QT Context Atom %s size = %ld\n",&id,len);
        		AdvanceStream(len - 8);
        		fileLength -= len;
				break;

//---------------------------------- Type of track
			
			case QT_vmhd:
				// do more setup stuff
				DEBUGCHUNK("QTChunk: QT_vmhd\n-----------------------------------\n");
				AdvanceStream(len - 8);
				fileLength -= len;
				
				fCurrentTrack->SetType(QT_VIDEO);
				fVideoTrackCount++;
								
				break;
			
			case QT_smhd:
				// sound - needs more setup stuff
				DEBUGCHUNK("QTChunk: QT_smhd\n-----------------------------------\n");
				AdvanceStream(len - 8);
				fileLength -= len;

				fCurrentTrack->SetType(QT_AUDIO);
				fAudioTrackCount++;
				break;				
					
			//---------------------- Sample Stuff
		
			case QT_stsd:
				// Sample Description Atom
				DEBUGCHUNK("QTChunk: QT_stsd\n");
				ReadSTSD(fCurrentTrack,len);
				fileLength -= len;				
				break;
				
			case QT_stsz:
				// Sample Size Information
				DEBUGCHUNK("QTChunk: QT_stsz\n");
		
				ReadSTSZ(len, fCurrentTrack);
				fileLength -= len;				
				break;
					
			case QT_stts:
				// Time to sample information
				DEBUGCHUNK("QTChunk: QT_stts\n");
				ReadSTTS(len, fCurrentTrack);

				fileLength -= len;
				break;
				
			case QT_stsc:
				// Sample to Chunk 
				ReadSTSC(	len,
							0, 0, 0,
							fCurrentTrack);

				fileLength -= len;
				break;
				
			case QT_stco:
				// Chunk offsets
				DEBUGCHUNK("QTChunk: QT_stco\n");				
				ReadSTCO(fCurrentTrack);

				fileLength -= len;
				break;
					
			case QT_stss:
				// sample sync info table (keyframes)
				DEBUGCHUNK("QTChunk: QT_stss (keyframes)\n");
			
				myErr = ReadSTSS(fCurrentTrack);			
				
				fileLength -= len;
				break;

//---------------------------------- Unknown
	      	default:
	      		_QTdebugprint("QT: Unknown chunk = %.4s len = %ld\n",&id,len);

				// check if we are within userdata	      		
				// the parser is LESS robust this way!!
				AdvanceStream(len - 8);
				fileLength -= len;
				break;    		  	
		}
	}

	_QTdebugprint("QT: Parse Finished\n");	

	if (fCurrentTrack) {
		// push last track
		fTrackList->AddItem(fCurrentTrack);
		ParseIndex(fCurrentTrack);
	}

	for(int i=0; i < TrackList()->CountItems(); i++) {
		QTTrack *track = (QTTrack *)TrackList()->ItemAt(i);
		if (track->Type() == QT_AUDIO) {
			audio_smp_details *auds = &track->audioCodecList[0];
			track->audioFrameCount = (track->dataSize / ((auds->bitsPerSample / 8) * auds->audioChannels));
		}
	}

	#ifdef DEBUGONE
	// check for video track, then check interframe gaps against track duration
	bigtime_t		gapCount = 0;
	QTTrack			*myTrack = (QTTrack *)TrackList()->ItemAt(0);
		
	if ((myTrack) && (myTrack->Type() == QT_VIDEO) && (myTrack->VIndex())) {
	
		for (uint32 i = 0;i < myTrack->videoFrameCount; i++) {
			gapCount += myTrack->VIndex()[i].start_time;
		}		
		gapCount = myTrack->VIndex()[myTrack->videoFrameCount-1].start_time;
	
		printf("QT: Gap time = %Ld (%.8f secs)\n",gapCount,(double)gapCount / 1000000.0);
		printf("QT: Gap drift +/- = %.8f secs\n",(double)gapCount / 1000000.0 - (float)myTrack->qt_tkhdr.duration / (float)myTrack->qt_tkhdr.timescale);
	}
	#endif

	return B_NO_ERROR;
}

status_t QTAtomHandler::ParseIndex(QTTrack *track)
{
	MediaIndex			*mediaIndex;

	_QTdebugprint("QTAtomHandler::ParseIndex()\n");

	switch (track->Type()) {
		case QT_VIDEO:
			return CreateVideoIndex(track);
			break;

		case QT_AUDIO:
			return CreateSoundIndex(fCurrentTrack);
			break;
	}
	return QT_ERROR;
}

status_t QTAtomHandler::ReadSTSD(QTTrack *track, uint32 len)
{
	switch (track->Type()) {
		case QT_VIDEO:
			return ReadVideoSTSD((int32)len - 8);
			break;
			
		case QT_AUDIO:
			return ReadAudioSTSD((int32)len - 8);
			break;
			
		default:
			AdvanceStream(len - 8);
			return QT_OK;
			break;
	}
	
	return QT_OK;
}

void QTAtomHandler::CheckTempBuffer(uint32 len)
{
	if (!fTempBuffer) {
		fTempBuffer = malloc(len+1);
		fBufferSize = len+1;	
	}

	if (len > fBufferSize) {
		if (fTempBuffer) {
			free(fTempBuffer);
		}
		
		fTempBuffer = malloc(len+1);
		fBufferSize = len+1;	
	}

	fRealBuffer = (uint8 *)fTempBuffer;
}

size_t QTAtomHandler::ParseUserData(size_t currentLen)
{
	size_t			atomLength;
	uint32			atomCode;
	ssize_t			userData;
	
	userData = currentLen - 8;

	if (userData == 4) {
		ReadMSB32();
		userData -= 4;
	}

	while (userData > 0) {
		
		atomLength = ReadMSB32();
		atomCode = ReadMSB32();

		userData -= atomLength;
	
		switch (atomCode) {
			case 'name':
				CheckTempBuffer(atomLength-8);
				// XXXdbg inStream->Read(fTempBuffer,atomLength-8);
				ReadChunk(fTempBuffer,atomLength-8);
				fRealBuffer[atomLength-8] = 0;
				
				if (fCurrentTrack) fCurrentTrack->SetName((char *)fRealBuffer);
				
				if (ReadMSB32() != 0) {
					RewindStream(4);
				} else userData -= 4;		
				break;

			case 0xA9637079: // '©cpy':
				int len, junk;

				#ifdef DEBUGONE
				printf("QT_Chunk: QT_%4s Len = %ld\n",&atomCode,atomLength);
				#endif

				len  = ReadMSB16();
				junk = ReadMSB16();

				fCopyright = (char *)malloc(len+1);
				if (fCopyright) {
					ReadChunk(fCopyright, atomLength - 12);
					fCopyright[atomLength - 12] = '\0';
				} else {
					CheckTempBuffer(atomLength-12);
					ReadChunk(fTempBuffer,atomLength-12);
					fRealBuffer[atomLength-12] = 0;
				}

				if (ReadMSB32() != 0) {
					RewindStream(4);
				} else {
					userData -= 4;
				}
				break;

			case 0xA9696E66: // 'inf':
			case 0xA9646179: // 'day':
			case 0xA9666d74: // fmt	
			case 0xA9777274: { // wrt
				// read to buffer
				#ifdef DEBUGONE
				printf("QT_Chunk: QT_%4s Len = %ld\n",&atomCode,atomLength);
				#endif
				CheckTempBuffer(atomLength-8);
				// XXXdbg inStream->Read(fTempBuffer,atomLength-8);
				ReadChunk(fTempBuffer,atomLength-8);
				fRealBuffer[atomLength-8] = 0;
				if (ReadMSB32() != 0) {
					RewindStream(4);
				} else {
					userData -= 4;
				}
				break;
			}
						
			case 'WLOC':
				fXPos = ReadMSB16();
				fYPos = ReadMSB16();

				#ifdef DEBUGONE
				printf("QTChunk: QT_WLOC Len = %ld (-8 = %ld)\n",atomLength,atomLength-8);
				printf("QTChunk: XPos = %ld YPox = %ld\n",fXPos,fYPos);
				#endif
				if (ReadMSB32() != 0) {
					RewindStream(4);
				} else userData -= 4;		
				break;

			case 'LOOP': {
				int32 loopvalue = ReadMSB32();
				_QTdebugprint("QTChunk: QT_LOOP, val = %ld\n",loopvalue);
				if (ReadMSB32() != 0) {
					RewindStream(4);
				} else userData -= 4;		
				break;
			}

			case 'ptv ': {
				int32 ptvvalue = ReadMSB32();
				ReadMSB32();
				_QTdebugprint("QTChunk: QT_ptv , val = %ld\n",ptvvalue);
				if (ReadMSB32() != 0) {
					RewindStream(4);
				} else userData -= 4;		
				break;
			}
										
			case 'SelO':  {
				ReadMSB8(); 
				if (ReadMSB32() != 0) {
					RewindStream(4);
				} else userData -= 4;		
				break;
			}
			
			case 'AllF': {
				ReadMSB8(); 
				if (ReadMSB32() != 0) {
					RewindStream(4);
				} else userData -= 4;		
				break;
			}
		
			default:

				#ifdef DEBUGONE
	      		printf("QT: Unknown userData = %.4s len = %ld\n",&atomCode,atomLength);
	      		//printf("userData = %ld\n",userData);
	      		#endif
				
				// unknown type of userdata
				AdvanceStream(atomLength - 8);
				
				// check for a blank space
				atomLength = ReadMSB32();
				if (atomLength != 0) {
					// go back
					RewindStream(4);
				} else {
					userData -= 4;
				}
				
				break;
		}
	}
	
	return currentLen;
}

void QTAtomHandler::ReadMVHD(qt_mvhd_details *header)
{  	
	int32		i;

  	header->version 		=	ReadMSB32();
  	header->creation 		=	ReadMSB32();
  	header->modtime 		=	ReadMSB32();
  	header->timescale 		=	ReadMSB32();
  	header->duration 		=	ReadMSB32();
  	header->rate 			=	ReadMSB32();
  	header->volume 			=	ReadMSB16();
  	header->r1  			=	ReadMSB32();
  	header->r2  			=	ReadMSB32();
  	for(i=0;i<3;i++) 
  		for(int32 j=0;j<3;j++) 
			header->matrix[i][j] = ReadMSB32();

  	header->r3  			=	ReadMSB16();
  	header->r4  			=	ReadMSB32();
  	header->pv_time 		=	ReadMSB32();
  	header->post_time 		=	ReadMSB32();
  	header->sel_time		=	ReadMSB32();
  	header->sel_durat 		=	ReadMSB32();
  	header->cur_time 		=	ReadMSB32();
  	header->nxt_tk_id 		=	ReadMSB32();

	_QTdebugprint("MVHD: version 0x%x creation 0x%x modtime 0x%x timescale %d duration %d\n",
			   header->version,header->creation,header->modtime,header->timescale,header->duration);
	_QTdebugprint("MVHD: rate %d volume %d r1 %d r2 %d r3 %d r4 %d pv_time %d sel time %d\n",
			   header->rate,header->volume,header->r1,header->r2,header->r3,header->r4,
			   header->pv_time,header->sel_time);
	_QTdebugprint("MVHD: matrix %d %d %d ## %d %d %d ## %d %d %d\n",
				header->matrix[0][0],header->matrix[0][1],header->matrix[0][2],
				header->matrix[1][0],header->matrix[1][1],header->matrix[1][2],
				header->matrix[2][0],header->matrix[2][1],header->matrix[2][2]);

  	if (header->timescale != 0) fMVTimescale = header->timescale;
  	else {
  		fMVTimescale = 1000;
  		header->timescale = 1000; // SC ?
  	}
  	
  	header->rate = header->rate >> 16;
}

bool QTAtomHandler::ReadTKHD(qt_tkhd_details *header)
{
	int32		i, j;

	// version includes flags
  	header->version 		=	ReadMSB8();
  	header->trackv1			=	ReadMSB8();
   	header->trackv2			=	ReadMSB8();
  	header->trackv3			=	ReadMSB8();
  	
  	header->creation 		=	ReadMSB32();
  	header->modtime 		=	ReadMSB32();
  	header->trackid 		=	ReadMSB32();
  	header->timescale 		=	ReadMSB32();
  	header->duration 		=	ReadMSB32();
  	header->time_off 		=	ReadMSB32();
  	header->priority  		=	ReadMSB32();
  	header->layer  			=	ReadMSB16();
  	header->alt_group 		= 	ReadMSB16();
  	header->volume  		=	ReadMSB16();
  	
  	for(i=0;i<3;i++) 
  		for(j=0;j<3;j++) 
			header->matrix[i][j] = ReadMSB32();
  
  	header->tk_width 		=	ReadMSB32();
  	header->tk_height 		=	ReadMSB32();
  	header->pad  			=	ReadMSB16();

	// if there is no timescale, use global scale
 	if (header->timescale == 0)
		header->timescale = fMVTimescale;

	#ifdef DEBUG_MOVIEINFO
	printf("QT: TKHD ID %ld Timescale %ld\n",
		header->trackid,header->timescale);
	printf("         True Duration = %.2f secs\n",(float)header->duration / (float)header->timescale);
	printf("		 Volume = %ld\n", header->volume);

	_QTdebugprint("TKHD: version 0x%x creation 0x%x modtime 0x%x timescale %d duration %d\n",
			   header->version,header->creation,header->modtime,header->timescale,header->duration);
	_QTdebugprint("TKHD: priority %d layer %d volume %d width %d height %d\n",
			   header->priority,header->layer, header->volume,
			   header->tk_width, header->tk_height);
	_QTdebugprint("TKHD: matrix %d %d %d ## %d %d %d ## %d %d %d\n",
				header->matrix[0][0],header->matrix[0][1],header->matrix[0][2],
				header->matrix[1][0],header->matrix[1][1],header->matrix[1][2],
				header->matrix[2][0],header->matrix[2][1],header->matrix[2][2]);
	#endif


	fCurrentTrack->imageWidth = header->tk_width;
	fCurrentTrack->imageHeight = header->tk_height;
	
	// here we return if the track is a valid movie one, not
	// a preview one for example
	
	return true;
}

void QTAtomHandler::ReadMDHD(qt_mdhd_details *header)
{
 	header->version 		=	ReadMSB32();
  	header->creation 		=	ReadMSB32();
  	header->modtime 		=	ReadMSB32();
  	header->timescale 		=	ReadMSB32();
  	header->duration 		=	ReadMSB32();
  	header->language 		=	ReadMSB16();
  	header->quality 		=	ReadMSB16();

	if (!header->timescale)
		header->timescale = fCurrentTrack->qt_tkhdr.timescale;
	
	_QTdebugprint("QT MDHD: Time = %.2f secs\n",(float)header->duration / (float)header->timescale);
	_QTdebugprint("QT MDHD: Timescale = %ld Duration = %ld\n",header->timescale,header->duration);
}

void QTAtomHandler::ReadHDLR(qt_hdlr_details *header, int32 len)
{
	// media header component data
 	ReadMSB32(); // version and flags
  	header->type 			=	ReadMSB32();
  	header->subtype 		=	ReadMSB32();
  	header->vendor 			=	ReadMSB32();
  	header->flags 			=	ReadMSB32();
  	header->mask 			=	ReadMSB32();
  	
  	#ifdef DEBUG_MOVIEINFO
	printf("HDLR: type 0x%x subtype 0x%x vendor %.4s (0x%x) flags 0x%x mask 0x%x\n",
		   header->type, header->subtype, &header->vendor, header->vendor,
		   header->flags, header->mask);

 	printf("QT HDLR: Len remaing = %ld\n",len-32);
  	#endif
  	
  	if (len > 32) {
  		len -= 32;
  		ReadName(len);
	}
}

status_t QTAtomHandler::ReadLOAD(QTTrack *inputTrack)
{
	// track cache details

	inputTrack->loadStart 		= ReadMSB32();
	inputTrack->loadDuration	= ReadMSB32();
	
	if (ReadMSB32() > 0) inputTrack->loadInfo = true;
	else inputTrack->loadInfo = false;
	
	inputTrack->loadDescription	= ReadMSB32();

	return QT_OK;
}	
	
void QTAtomHandler::ReadDREF()
{
	int32		i, num, r;
	int32		size, type;

	ReadMSB32(); // go past version and flags
	num = ReadMSB32(); // number of refs
		
	for (i = 0;i<num;i++) {
		size = ReadMSB32(); // size
		type = ReadMSB32(); // type
		ReadMSB32(); // flags et al
		
		// print details of the alias
		#ifdef DEBUGONE
		switch (type) {
			case 'alis': printf("QT_dref: Mac alias\n"); break;
			case 'rsrc': printf("QT_dref: Mac rsrc\n"); break;
		}
		printf("QT_ref: data remain %ld\n",size-12);
		#endif
		
		// read in remaining pad bytes
		for (r = 0;r<size-12;r++)
			ReadMSB8();
	}
}

void QTAtomHandler::ReadName(int32 rlen)
{
	int32		len, i;

	_QTdebugprint("Encoded by ");

	len = ReadMSB8();
	rlen--;
	
	if (!rlen) 
		rlen = len;
  	  	
  	for (i=0;i<rlen;i++) {
  		ulong d = ReadMSB8() & 0x7f;
		_QTdebugprint("%c",d);
	}
		
	_QTdebugprint("\n");
}

void QTAtomHandler::ReadELST(QTTrack *inputTrack)
{
	uint32 			num, i;
	uint32			duration, time, rate;

	ReadMSB32(); // version and flags
	num	= ReadMSB32();

	inputTrack->InitEditList(num);
	
	for (i = 0;i < num;i++) {
		duration= ReadMSB32(); 
   		time 	= ReadMSB32(); 
   		rate 	= ReadMSB32(); 

		inputTrack->AddEditListEntry(duration,time,rate);

		_QTdebugprint("ELST: duration %ld time %ld rate %ld\n",duration,time,rate);

   		if (i==0) {
   			if (time == 0xffffffff)	inputTrack->initDuration += duration;
      		else if (time != 0x0)	inputTrack->startOffset += time;
   		}
	}
}

status_t QTAtomHandler::ReadVideoSTSD(int32 atom_size)
{
	// read in video codec information
	uint32 					codecCount, i, currentCodec;
	video_smp_details		*tempDetails;
	
	ReadMSB32(); // version and flags
  	codecCount	= ReadMSB32();

	if (fCurrentTrack->videoCodecList) {
		// we need to resize the index contained within this track
		tempDetails = (video_smp_details *)malloc(codecCount * sizeof(video_smp_details));
		memset(tempDetails, 0, codecCount * sizeof(video_smp_details));
		currentCodec = fCurrentTrack->videoCodecCount;	
		memcpy(tempDetails, fCurrentTrack->videoCodecList, fCurrentTrack->videoCodecCount * sizeof(video_smp_details));
		
		// free old codec details
		free(fCurrentTrack->videoCodecList);
		
		fCurrentTrack->videoCodecList = tempDetails;
		
	} else {
		// create an index
		fCurrentTrack->videoCodecList = (video_smp_details *)malloc(codecCount * sizeof(video_smp_details));
		memset(fCurrentTrack->videoCodecList, 0, codecCount * sizeof(video_smp_details));
		currentCodec = 0;	
	}

	fCurrentTrack->videoCodecCount = codecCount;

	_QTdebugprint("QT: Video Codecs:%ld\n",codecCount);

	for (i = 0; i < codecCount; i++) {
		ReadVideoDescription(&fCurrentTrack->videoCodecList[i], atom_size);
	}

	return QT_OK;
}

status_t QTAtomHandler::ReadVideoDescription(video_smp_details *videoDetails, int32 atom_size)
{
	size_t		length;

	// size of sample description
  	length							= ReadMSB32();
  	videoDetails->codecID			= ReadMSB32(); // "compression format or media type"

  	_QTdebugprint("QT Image STSD: Length of this STSD entry:%ld\n",length);

  	// these are 6 reserved bytes
  	ReadMSB32();
  	ReadMSB32();
  	ReadMSB16();

  	// sample description id
	videoDetails->sampleDesc 		= ReadMSB16();
	
	videoDetails->codecVendor 		= ReadMSB32();
	videoDetails->temporalQuality 	= ReadMSB32();
	videoDetails->spatialQuality 	= ReadMSB32();
	
	videoDetails->width 			= ReadMSB16();
	videoDetails->height	 		= ReadMSB16();

	videoDetails->horRes			= ReadMSB16();
	ReadMSB16(); // unk4
	videoDetails->vertRes			= ReadMSB16();
	ReadMSB16(); // unk5
	ReadMSB32(); // unk6
	ReadMSB16(); // unk7
	
  	ReadName(32);

	videoDetails->depth 			= ReadMSB16();
	videoDetails->flags		 		= ReadMSB16();
  	
  	length -= 86;   /* because we read more than the actual length */
  	
	#ifdef DEBUG_MOVIEINFO
	// display information about this track
	printf("QT: Codec Header values (codec id %.4s (0x%x) vendor 0x%x\n",
		   &videoDetails->codecID, videoDetails->codecID, videoDetails->codecVendor);
	printf("QT: Movie Depth %ld Sample Des ID %ld Temp Qual %ld Spat Qual %ld \n",videoDetails->depth,
			videoDetails->sampleDesc,videoDetails->temporalQuality,videoDetails->spatialQuality);
	printf("QT: Width %ld Height %ld\n",videoDetails->width,videoDetails->height);
	#endif


 	if ((videoDetails->depth == 8) || (videoDetails->depth == 40) || 
 		(videoDetails->depth == 4) || (videoDetails->depth == 36)) {

		ProcessColourMap(videoDetails, length);
	} else {
		if (length > 0) {
			// read in codec specific data
			_QTdebugprint("QT STSD: Reading video specific data:%ld\n",length);
			videoDetails->codecData = malloc(length);
			if (videoDetails->codecData) {
				if (ReadChunk(videoDetails->codecData, length) != length) {
					_QTdebugprint("error reading video codec details\n");
					return QT_ERROR;
				}
			}		
			videoDetails->codecDataLength = length;
		}
	}
	
	// weird colour map stuff
  	if (videoDetails->depth > 32) {
  		videoDetails->depth = 8; 
	}

	return QT_OK;		
}

status_t QTAtomHandler::ProcessColourMap(video_smp_details *videoDetails, size_t length)
{
	uint32		startMap, cFlags, endMap;
	uint32		r, g, b, p, i;

 	if ((videoDetails->depth == 8) || (videoDetails->depth == 40) || (videoDetails->depth == 4) || (videoDetails->depth == 36)) {
	
		if (videoDetails->depth & 0x04) videoDetails->colourCount = 16;
    	else videoDetails->colourCount = 256;

		_QTdebugprint("QT: Colour map found at depth of %ld with %ld colours\n",videoDetails->depth,videoDetails->colourCount);

		videoDetails->colourMap = (rgb_color *)malloc(videoDetails->colourCount*sizeof(rgb_color));
		if (!videoDetails->colourMap)
			return QT_ERROR;
		
		// check for map
		if (!(videoDetails->flags & 0x08) && (length)) {	
			DEBUGTEXT("QT: Alien colour map\n");
			startMap 	= ReadMSB32();
	      	cFlags 		= ReadMSB16();
	      	endMap   	= ReadMSB16();
	      	length -= 8;
	
			for (i = startMap; i <= endMap;i++) {
				p = ReadMSB16();
				r = ReadMSB16();
				g = ReadMSB16();
				b = ReadMSB16();
				
				length -= 8;
				if (cFlags & 0x8000) p = i;
						
				if (p < videoDetails->colourCount) {
		  			videoDetails->colourMap[p].red = r;
		  			videoDetails->colourMap[p].green = g;
		  			videoDetails->colourMap[p].blue = b;
					videoDetails->colourMap[p].alpha = (uint8)255;
				}
				if (length <= 0) break;	
			}				
			DEBUGTEXT("QT: Got colour map\n");
		} else {
			if (videoDetails->colourCount == 256)
				CreateAppleColourMap(videoDetails->colourMap);
		}
	} 

	return QT_OK;
}

status_t QTAtomHandler::ReadAudioSTSD(int32 atom_size)
{
	uint32 					currentCodec;
	uint32 					codecCount;
	uint32					i;
	audio_smp_details		*tempDetails;

	ReadMSB32(); // version and flags
  	codecCount = ReadMSB32();

	if (fCurrentTrack->audioCodecList) {
		// we need to resize the index contained within this track
		tempDetails = (audio_smp_details *)malloc(codecCount * sizeof(audio_smp_details));
		memset(tempDetails, 0, codecCount * sizeof(audio_smp_details));
		currentCodec = fCurrentTrack->audioCodecCount;	
		memcpy(tempDetails, fCurrentTrack->audioCodecList, fCurrentTrack->audioCodecCount * sizeof(audio_smp_details));
		
		// free old codec details
		free(fCurrentTrack->audioCodecList);
		
		fCurrentTrack->audioCodecList = tempDetails;
		
	} else {
		// create an index
		fCurrentTrack->audioCodecList = (audio_smp_details *)malloc(codecCount * sizeof(audio_smp_details));
		memset(fCurrentTrack->audioCodecList, 0, codecCount * sizeof(audio_smp_details));
		currentCodec = 0;	
	}

	fCurrentTrack->audioCodecCount = codecCount;

	_QTdebugprint("QT: Audio Codecs:%ld\n",codecCount);

	for (i = 0; i < codecCount; i++) {
		ReadAudioCodecHDR(&fCurrentTrack->audioCodecList[i], atom_size);
	}

	return B_OK;
}

status_t QTAtomHandler::ReadAudioCodecHDR(audio_smp_details *audioDetails, int32 atom_size)
{
 	size_t 		length;
	off_t		startPosition;
	
	startPosition = (fHeaderPosition - fHeaderBuffer);
  	  	
  	length = ReadMSB32();
  	
  	audioDetails->codecID 		= ReadMSB32();
  	ReadMSB32(); // data reference id
  	audioDetails->codecVersion 	= ReadMSB32();
 	audioDetails->codecRevision = ReadMSB32();
 	audioDetails->codecVendor 	= ReadMSB32();
 	audioDetails->audioChannels = ReadMSB16();
  	audioDetails->bitsPerSample	= ReadMSB16();
  	
  	if (audioDetails->codecVendor == 0)
  		audioDetails->codecVendor = 'appl';
  	
  	_QTdebugprint("QT STSD: AudioSTSD length:%ld (codec %.4s (0x%x) version %d vendor 0x%x)\n",
			   length, &audioDetails->codecID, audioDetails->codecID,
			   audioDetails->codecVersion, audioDetails->codecVendor);
	_QTdebugprint("QT Audio: #channels %d BitsPerSample = %ld\n",
			   audioDetails->audioChannels, audioDetails->bitsPerSample);

	if ((audioDetails->bitsPerSample == 1) && 	((audioDetails->codecID == QT_twos) 	||
												(audioDetails->codecID == QT_raw) 	||
												(audioDetails->codecID == QT_raw00))) {
		// bits per sample is incorrect with a raw file
		audioDetails->bitsPerSample = 8;						
	}
	
	ReadMSB16(); // comp id										
	
	audioDetails->audioPackSize 	= ReadMSB16();
	audioDetails->audioSampleRate	= ReadMSB16();

	ReadMSB16(); // pad
	
	// 36 == the amount of data we just read.  anything extra is codec specific data
	// that should be read.
	length -= 36;

	if (length > 0) {
		// codec specific data needs to be read in
		audioDetails->codecDataLength = length;
		audioDetails->codecData = malloc(length);
		
		if (!audioDetails->codecData)
			return QT_ERROR;
	
		_QTdebugprint("QT STSD: Reading in %ld bytes codec specific\n", length);
		
		if (ReadChunk(audioDetails->codecData, length) != length) {
			_QTdebugprint("QT STSD: Unable to read codec specific data\n");
			return QT_ERROR;
		}
	}

	// calculate bytes per sample
	
	switch (audioDetails->bitsPerSample) {
		case 8: 	audioDetails->bytesPerFrame = 1; break;
		case 16:	audioDetails->bytesPerFrame = 2; break;
		case 32:	audioDetails->bytesPerFrame = 4; break;
		default:	audioDetails->bytesPerFrame = 0; _QTdebugprint("QT STSD: Unable to calc bytes per frame\n"); break;
	}

	if (audioDetails->audioChannels == 2)
		audioDetails->bytesPerFrame *= 2;
		
	
	#ifdef DEBUG_MOVIEINFO
	printf("QT: Audio Codec ");
	PrintAudioType(audioDetails->codecID);
	printf(" Rate:%ld Channels:%ld BitPS:%ld",audioDetails->audioSampleRate,audioDetails->audioChannels,audioDetails->bitsPerSample);
	printf(" Pack Size:%ld Converted BytesPF:%ld\n",audioDetails->audioPackSize,audioDetails->bytesPerFrame);
	#endif

	return QT_OK;
}

void QTAtomHandler::PrintAudioType(uint32 type)
{
	switch (type) {
		case QT_raw:
			printf("PCM (raw)");
			break;
		case QT_raw00:
			printf("PCM0 (raw00)");
			break;
		case QT_twos:
			printf("TWOS");
			break;
		case QT_MAC6:
			printf("MAC6");
			break;
		case QT_ima4:
			printf("IMA4");
			break;
		default:
			printf("Unknown audio codec");
			break;
	}
}

void QTAtomHandler::CreateAppleColourMap(rgb_color *colourMap)
{
	uint32			r, g, b, i, d, ip;
	static uint8 	colourPat[10] = { 0xee, 0xdd, 0xbb, 0xaa, 0x88, 0x77, 0x55, 0x44, 0x22, 0x11};

    r = g = b = 0xff;

	DEBUGTEXT("QT: Build MAC Colour Map\n");

    for (i = 0;i < 215;i++) {
		colourMap[i].red = 0x101 * r;
      	colourMap[i].green = 0x101 * g;
      	colourMap[i].blue = 0x101 * b;
      	colourMap[i].alpha = (uint8)255;
      	b -= 0x33;
      	if (b < 0) { b = 0xff; g -= 0x33; if (g < 0) { g = 0xff; r -= 0x33; } }
    }
    
    for (i = 0;i < 10;i++) {
    	d = 0x101 * colourPat[i];
      	ip = 215 + i; 
      
      	colourMap[ip].red = d;
      	colourMap[ip].green = colourMap[ip].blue = 0;
      	ip += 10;

      	colourMap[ip].green = d;
      	colourMap[ip].red = colourMap[ip].blue = 0;
      	ip += 10;

      	colourMap[ip].blue = d;
      	colourMap[ip].red = colourMap[ip].green = 0;
      	ip += 10;

		colourMap[ip].red = colourMap[ip].green = colourMap[ip].blue = d;
    }

    colourMap[255].red = colourMap[255].green = colourMap[255].blue = 0;
}

status_t QTAtomHandler::ReadSTTS(size_t length, QTTrack *inputTrack)
{
	uint32		number, i, sampleCount, duration, current;
	uint32		totalDuration;
	
	totalDuration = 0;
	
  	ReadMSB32(); // version and flags
  	number	= ReadMSB32();  
  	length -= 8;

  	_QTdebugprint("QT STTS: Entries:%ld CEntries:%ld\n", number, length >> 3);
  	//number = length >> 3; // recalc number
  	
  	inputTrack->sttsTable = (qt_stts_details *)malloc(number * sizeof(qt_stts_details));
	
	if (!inputTrack->sttsTable)
		return QT_ERROR;

	current = 0;	

	inputTrack->sttsEntryCount = number;

	for (i = 0;i < number;i++) { 
    	sampleCount	= ReadMSB32();
    	duration	= ReadMSB32();  

    	length -= 8;
    	
    	// adjust if same value
    	if (duration == 0) 
    		duration = 1;
   
   		inputTrack->sttsTable[current].cnt = sampleCount;
    	inputTrack->sttsTable[current].time = (float)fCurrentTrack->qt_mdhdr.timescale / (double)duration;
       
    	//printf("QT STTS %ld: Sam No %ld, Duration %ld Time %lf TT = %.4f\n",
		//	   i, sampleCount, duration, inputTrack->sttsTable[current].time,
		//	   (float)1000000.0 / (double)inputTrack->sttsTable[current].time);
    	current++;
  		
		totalDuration += duration;
  	}

	if (number == 1)
		_QTdebugprint("QT STTS: So, all of our frames are the SAME time length\n");

	//printf("QT STTS: Total Duration = %ld Total Samples = %ld\n",totalDuration,i);
  	  	
	return QT_OK;
}

status_t QTAtomHandler::ReadSTSC(	size_t 			length, 
									uint32 			chunkOff, 
									uint32 			codecCount, 
									uint32 			codec1Num, 
									QTTrack 		*inputTrack)
{

	// sample to chunk table - maps samples to correct chunks (Page 51)	
	uint32		count = 0, lastEntry, i;
	bool		oldSkool;
	uint32		firstChunk, samplesPerChunk, chunkTag;
	uint32		current;
 
  	ReadMSB32(); // version and flags
  	count = ReadMSB32();	

	_QTdebugprint("QT STSC : Entries:%ld Length:%ld\n",count,length);

  	length -= 16;

  	i = count ? (length / count) : 0;
  	
  	if (i == 16) {
		_QTdebugprint("QT STSC : Old skool type \n");
		length -= count * 16; 
		oldSkool = true;
	} else {
		_QTdebugprint("QT STSC : New type\n");
		length -= count * 12;
		oldSkool = false;
	}

	inputTrack->stscTable = (qt_stsc_details *)malloc((count + 1) * sizeof(qt_stsc_details));
	
	if (!inputTrack->stscTable)
		return QT_ERROR;
		
	inputTrack->stscEntryCount = count;

	current = lastEntry = 0;

	for (i = 0;i < count;i++) {

		if (oldSkool) { // 4 entries
			firstChunk 		= ReadMSB32();
			ReadMSB32(); // not needed
			samplesPerChunk	= ReadMSB32();
			chunkTag 		= ReadMSB32();

	      	if (i > 0) inputTrack->stscTable[current-1].num = firstChunk - lastEntry;
		
			lastEntry = firstChunk;
			
			if (i == (count - 1)) {
				_QTdebugprint("QT STSC: No STGS chunk warning\n");
				inputTrack->stscTable[current].num = 100000;
			}
			
		} else { // 3 entries
			firstChunk 		= ReadMSB32();
			samplesPerChunk	= ReadMSB32();
			chunkTag 		= ReadMSB32();
		
			inputTrack->stscTable[current].num   = samplesPerChunk;
		}

	    inputTrack->stscTable[current].first = firstChunk - 1 + chunkOff;

    	if (chunkTag > (codecCount - chunkOff)) {
	 		samplesPerChunk = chunkTag = 1; 
	 	}
    	
    	inputTrack->stscTable[current].tag = chunkTag - 1 + codec1Num;
    	current++;
	}		

  	inputTrack->stscTable[current].first = 0;
  	inputTrack->stscTable[current].num   = 0;
  	inputTrack->stscTable[current].tag   = 0;

  	while (length > 0) { 
  		ReadMSB8();
  		length--; 
  	}
    	
  	return QT_OK;
}

status_t QTAtomHandler::ReadSTSZ(size_t atomLength, QTTrack *inputTrack)
{
	// sample size table - locate individual sample sizes (Page 53)

	uint32		sampleSize;
	uint32		entries, count, i, samplePosition = 0;

	ReadMSB32(); // version and flags
	sampleSize = ReadMSB32();
	entries = count = ReadMSB32();	

   	_QTdebugprint("QT STSZ: Len:%ld Sample Size:%ld Entries:%ld\n",atomLength,sampleSize,count);

	// calculate number of entries from size of atom
	atomLength = (atomLength - 20) / 4;   // number of stored samples

	if (atomLength == 0)
		count = 1;
	
	inputTrack->stszTable = (uint32 *)malloc(count * sizeof(uint32));
	
	if (!inputTrack->stszTable)
		return QT_ERROR;
	
	inputTrack->stszEntryCount = count;
	
	for (i = 0; i < count; i++) {
		if (i < atomLength)
			inputTrack->stszTable[samplePosition] = ReadMSB32();
		else if (i == 0)
			inputTrack->stszTable[samplePosition] = sampleSize;
		else
			inputTrack->stszTable[samplePosition] = inputTrack->stszTable[samplePosition-1];
    	samplePosition++;
  	}
	
	switch (inputTrack->Type()) {
		case QT_VIDEO: inputTrack->videoFrameCount = entries; break;
		case QT_AUDIO: inputTrack->audioFrameCount = inputTrack->stcoEntryCount; break;
	}
	
	return QT_OK;
}

status_t QTAtomHandler::ReadSTCO(QTTrack *inputTrack)
{
	uint32		entryCount, i;
	
	ReadMSB32(); // version and flags
	entryCount = ReadMSB32();

	_QTdebugprint("QT STCO: Entries %ld\n",entryCount);

	// we should only one stco per track
	if (!inputTrack->stcoTable)
		free(inputTrack->stcoTable);

	inputTrack->stcoEntryCount = entryCount;
	inputTrack->stcoTable = (uint32 *)malloc(entryCount * sizeof(uint32));

	if (!inputTrack->stcoTable)
		return QT_ERROR;

	for (i = 0; i < entryCount; i++) {
		inputTrack->stcoTable[i] = ReadMSB32();
	}

	return QT_OK;
}

status_t QTAtomHandler::ReadSTSS(QTTrack *qtTrack)
{
	// sync samples table - maps key frames to samples (Page 50)
  	uint32 		num, i;
	uint32		*keyOffsets = 0;
  	
  	ReadMSB32(); // version and flags
  	num	= ReadMSB32();	

	_QTdebugprint("QT STSS : Entries %ld\n",num);

	keyOffsets = (uint32 *)malloc(num * sizeof(uint32));
	if (!keyOffsets) return QT_ERROR;
	
	for (i = 0;i<num;i++) {
		keyOffsets[i] = ReadMSB32();
		if(i == 0 && keyOffsets[i] != 1) {
			_QTdebugprint("QT STSS : first keyframe is not 1 (%ld)\n",keyOffsets[i]);
		}
	}

	qtTrack->stssTable = keyOffsets;
	qtTrack->stssEntryCount = num;

	return QT_OK;
}

status_t
QTAtomHandler::CreateVideoIndex(QTTrack *qtTrack)
{	
	uint32 				cur_s2chunk = 0;
	uint32 				nxt_t2samp, cur_t2samp, nxt_s2chunk, tag;
	uint32 				cur_off = 0, cur_samp = 0;
	uint32 				ix = 0;
	uint32 				gap;
	uint32				i, kf_index;
	MediaIndex 			*vidIdx; 
	uint32				num_samps, size, chunk_off;
	bigtime_t           sum = 0;

	if (qtTrack->stscTable == NULL) {
		printf("no stsc table?\n");
		return QT_ERROR;
	}

	if (qtTrack->sttsTable == NULL) {
		printf("no stts table?\n");
		return QT_ERROR;
	}

	vidIdx = (MediaIndex *)malloc(qtTrack->videoFrameCount*sizeof(MediaIndex));
	if (vidIdx == NULL) {
		printf("failed to allocate %d bytes (%d frames)\n",
			   qtTrack->videoFrameCount, sizeof(MediaIndex));
		return QT_ERROR;
	}
	qtTrack->videoIndex = vidIdx;

	_QTdebugprint("QT: Video Sample Count:%ld\n",qtTrack->videoFrameCount);
  	nxt_t2samp = cur_t2samp = 0;
	nxt_s2chunk = qtTrack->stscTable[cur_s2chunk + 1].first;

	gap = (uint32)((double)1000000.0 / qtTrack->sttsTable[cur_t2samp].time);	

  	tag = qtTrack->stscTable[cur_s2chunk].tag;
	
  	if (qtTrack->sttsTable)
  		nxt_t2samp += qtTrack->sttsTable[cur_t2samp].cnt;
	
	for (i=0, kf_index=0; i < qtTrack->stcoEntryCount; i++) {

		chunk_off = qtTrack->stcoTable[i];

    	if ((i == nxt_s2chunk) && (cur_s2chunk+1 < qtTrack->stscEntryCount)) {
      		cur_s2chunk++;
      		nxt_s2chunk = qtTrack->stscTable[cur_s2chunk + 1].first;
    	}

		// from STSC index
    	num_samps = qtTrack->stscTable[cur_s2chunk].num;

		//printf("build idx %ld\n",num_samps);

	    if (qtTrack->stscTable[cur_s2chunk].tag >= qtTrack->videoCodecCount) {
      		_QTdebugprint("QT: Stream STSC Chunk Error:%ld Tag:%ld\n",cur_s2chunk,qtTrack->stscTable[cur_s2chunk].tag);
    	} else if (qtTrack->stscTable[cur_s2chunk].tag != tag) {  
   			tag =  qtTrack->stscTable[cur_s2chunk].tag;
    	}

	    cur_off = chunk_off;

//printf("QT: num_samps:%ld\n",num_samps);
				
    	while (num_samps--) {

 		    if ((cur_samp >= nxt_t2samp) && (cur_t2samp < qtTrack->sttsEntryCount)) {
      			cur_t2samp++;
			    nxt_t2samp += qtTrack->sttsTable[cur_t2samp].cnt;
				// calc new gap
				gap = (uint32)((double)1000000.0 / qtTrack->sttsTable[cur_t2samp].time);
			}
			
			if(cur_samp < qtTrack->stszEntryCount) {
		    	size = qtTrack->stszTable[cur_samp]; //qtv_samp_sizes[cur_samp];
			}
			else {
		    	size = qtTrack->stszTable[qtTrack->stszEntryCount-1]; //qtv_samp_sizes[cur_samp];
			}
 
			
			vidIdx[ix].start_time = sum;
			vidIdx[ix].entrypos = cur_off;
			vidIdx[ix].entrysize = size;
			if (kf_index < qtTrack->stssEntryCount &&
				(ix+1) == qtTrack->stssTable[kf_index]) {

				vidIdx[ix].flags = B_MEDIA_KEY_FRAME;
				kf_index++;
			} else {
				if(qtTrack->stssEntryCount == 0 || ix == 0)
					vidIdx[ix].flags = B_MEDIA_KEY_FRAME;
				else
					vidIdx[ix].flags = 0;
			}

			qtTrack->dataSize += size;

//printf("QT:%ld size:%ld gap:%ld\n",ix,size,gap);
			
			// find largest entry
			if (size > qtTrack->largestEntry) qtTrack->largestEntry = size;
			
			ix++;
						
			sum += gap;
	      	cur_off += size;
	      	cur_samp++;
      		if (cur_samp >= qtTrack->videoFrameCount) break;
		}		
	    if (cur_samp >= qtTrack->videoFrameCount) break;
	}

	// store the largest entry of any frames

	_QTdebugprint("QT Chunks:%ld Video Frames:%ld Keyframes:%ld\n",qtTrack->stcoEntryCount,qtTrack->videoFrameCount,qtTrack->stssEntryCount);

	return QT_OK;
}

status_t
QTAtomHandler::CreateSoundIndex(QTTrack *inputTrack)
{
	_QTdebugprint("QTAtomHandler::CreateSoundIndex() - chunkC:%ld\n",inputTrack->stcoEntryCount);

  	int32 					i;
  	int32 					cur_s2chunk = 0, nxt_s2chunk = -1;
	uint32					numberSamples;
	size_t					soundSize;
	off_t					chunkOffset;
	audio_smp_details		*codecDetails = 0;
	bigtime_t               time_sum = 0;
	int32					*ptr, ima4_divider = 0, ima4_multiplier = 0;

	inputTrack->audioIndex = (MediaIndex *)malloc(inputTrack->stcoEntryCount * sizeof(MediaIndex));

	if (!inputTrack->audioIndex)
		return QT_ERROR;

	// temp for one codec
	codecDetails = &inputTrack->audioCodecList[0];

	if (inputTrack->stscEntryCount > 1)
		nxt_s2chunk = inputTrack->stscTable[1].first;

_QTdebugprint("QT: inputTrack->stscEntryCount coming into CreateSoundIndex():%ld\n",inputTrack->stscEntryCount);

		if (codecDetails->codecID == 'ima4') {      // ick
			ptr = (int32 *)codecDetails->codecData;
			if (ptr) {
				ima4_divider    = B_BENDIAN_TO_HOST_INT32(ptr[0]);
				ima4_multiplier = B_BENDIAN_TO_HOST_INT32(ptr[1]);
			}
			
			if (ima4_divider <= 0 || ima4_divider > 4096 ||
				ima4_multiplier <= 0 || ima4_multiplier > 4096) {
				ima4_divider = 64;
				ima4_multiplier = 34;
			}
		}


	for (i = 0; i < (int32)inputTrack->stcoEntryCount; i++) {

    	if ((i == nxt_s2chunk) && ((cur_s2chunk+1) < (int32)inputTrack->stscEntryCount)) {
      		cur_s2chunk++;
      		nxt_s2chunk = inputTrack->stscTable[cur_s2chunk + 1].first;
//_QTdebugprint("QT: Audio NS Count bumped to:%ld\n",cur_s2chunk);
    	}

		numberSamples = inputTrack->stscTable[cur_s2chunk].num; // sttz
		soundSize = numberSamples * codecDetails->bytesPerFrame;
		
		bigtime_t time_for_samples;  /* in usecs */

		time_for_samples = (bigtime_t)(((double)soundSize * 1000000.0) /
			                (double)inputTrack->audioCodecList[0].audioSampleRate / 
			                (double)codecDetails->bytesPerFrame);

		chunkOffset = inputTrack->stcoTable[i];

		inputTrack->audioIndex[i].start_time = time_sum + time_for_samples;
		inputTrack->audioIndex[i].entrypos   = chunkOffset;
		if (codecDetails->codecID == 'ima4') {      // ick
			inputTrack->audioIndex[i].entrysize = (codecDetails->audioChannels * numberSamples * ima4_multiplier) / ima4_divider;			
		} else {
			inputTrack->audioIndex[i].entrysize = soundSize;
		}
		inputTrack->audioIndex[i].flags = 0;

		time_sum += time_for_samples;

		//_QTdebugprint("%d: pos:%ld size:%ld nS:%ld\n",i, inputTrack->audioIndex[i].entrypos, inputTrack->audioIndex[i].entrysize, numberSamples);
		
		if (soundSize > inputTrack->largestEntry)
			inputTrack->largestEntry = soundSize;
			
		inputTrack->dataSize += soundSize;		
	}

	_QTdebugprint("QT: Max Size:%ld Total data:%ld\n",inputTrack->largestEntry,inputTrack->dataSize);	

	return QT_OK;
}

QTTrack *QTAtomHandler::FindTrackByID(uint32 id)
{
	for (int32 i = 0;i<fTrackList->CountItems();i++) {
		QTTrack *track = (QTTrack *)fTrackList->ItemAt(i);
		if (track->TrackID() == id) return track;
	}
	return 0;
}

void _QTdebugprint(const char *format, ...)
{
	#ifdef DEBUGONE
	char	dst[4096] = "\0";
	va_list	v;

	va_start(v,format);
	vsprintf(dst,format,v);
	printf("%s", dst);
	#endif
}

status_t QTAtomHandler::ReadAcrossMedia(void			*toBuffer,
										BPositionIO 	*stream,
										QTTrack			*track,
										MediaIndex		*mediaIndex,
										size_t			offsetBytes,
										size_t			readBytes)
{
	int32 			i;
	size_t			endCurrentPos;
	uint8			*bufferPos;
	size_t			startPos = 0, endPos = 0;
	size_t			posFromFirstChunk = 0, posFromLastChunk = 0;
	int32			startChunkNum = -1, endChunkNum = -1;
	size_t			readFromFirstChunk = 0, readFromLastChunk = 0;
	MediaIndex		*currentChunk;
	size_t			readLeft = readBytes;
	size_t			bytesRead;

	endCurrentPos = offsetBytes + readBytes;
	
	if (endCurrentPos > track->dataSize) {
		readBytes = track->dataSize - offsetBytes;
		endCurrentPos = offsetBytes + readBytes;
		readLeft = readBytes;
	}
	
	for (i = 0; i < (int32)track->stcoEntryCount; i++) {
		currentChunk = &mediaIndex[i];
		
		endPos += currentChunk->entrysize;
		
		if ((offsetBytes >= startPos) && (offsetBytes < endPos)) {
			// found it entry
			startChunkNum = i;
		
			size_t premapsize = offsetBytes - startPos;
			size_t endmapsize = endPos - offsetBytes;

			if (endmapsize >= readBytes) {
				// whole request can be satified
				readFromFirstChunk = readBytes;
				posFromFirstChunk = currentChunk->entrypos + premapsize;
			} else {
				// nope
				readFromFirstChunk = endmapsize;
				posFromFirstChunk = currentChunk->entrypos + premapsize;
			}		
		}

		if ((endCurrentPos > startPos) && (endCurrentPos <= endPos)) {
			// found end position
			endChunkNum = i;
			if (startChunkNum == i) {
				// we are on the same chunk as start position, no need..
				break;
			} else {
				readFromLastChunk = 1;
				posFromLastChunk = currentChunk->entrypos;
			}
		}

		startPos += currentChunk->entrysize;
	}
	
	// check if we could find the right chunk
	if (startChunkNum < 0) {
		printf("QTAtomHandler::ReadAcrossMedia() - No Start Chunk!!! Fatal Error\n");
		return B_ERROR;
	}

	bufferPos = (uint8 *)toBuffer;
	
	// first bit of buffer
	bytesRead = stream->ReadAt(posFromFirstChunk, bufferPos, readFromFirstChunk);
		
	bufferPos += bytesRead;
	readLeft -= bytesRead;
	
	if (readLeft > 0) {
		// do we need to read any chunks
		int32 readchunks = endChunkNum - startChunkNum;
		
		if (readchunks > 0) {
			for (i = startChunkNum + 1; i < endChunkNum;i++) {
				currentChunk = &mediaIndex[i];

				bytesRead = stream->ReadAt(currentChunk->entrypos, bufferPos, currentChunk->entrysize);
				bufferPos += bytesRead;
				readLeft -= bytesRead;
			}
		}
				
		if (readLeft > 0) {
			// we do we need to read a last bit?
			bytesRead = stream->ReadAt(posFromLastChunk, bufferPos, readLeft); 
			bufferPos += bytesRead;
		}
	}

	return B_OK;
}

//
//	Low level reading
//
size_t QTAtomHandler::ReadChunk(void *dstbuff, size_t len)
{
	if (fDecompressedBuffer) {
		if ((size_t)(fDecompressedPosition + len - fDecompressedBuffer) < fDecompressedSize) {
			memcpy(dstbuff, fDecompressedPosition, len);
			fDecompressedPosition += len;
		
			return len;
		}
	} else {
		if ((size_t)(fHeaderPosition + len - fHeaderBuffer) < fMoovSize) {
			memcpy(dstbuff, fHeaderPosition, len);
			fHeaderPosition += len;
		
			return len;
		}
	}
	
	return 0;
}

uint32 QTAtomHandler::ReadMSB32()
{
	if (fDecompressedBuffer) {
		// Read decompressed header information
		if ((size_t)(fDecompressedPosition - fDecompressedBuffer) < fDecompressedSize) {
			uint32		value = *((uint32 *)fDecompressedPosition);
			fDecompressedPosition += 4;
		
			return B_BENDIAN_TO_HOST_INT32(value);
		} else {
			_QTdebugprint("32: TRIED TO READ BEYOND END OF DECOMPRESSED HEADER (hdrPos %ld, size %d)\n",
					   (ulong)(fHeaderPosition - fHeaderBuffer), fMoovSize);
		}
	} else {
		// Read uncompressed header information
		if ((size_t)(fHeaderPosition - fHeaderBuffer) < fMoovSize) {
			uint32		value = *((uint32 *)fHeaderPosition);
			fHeaderPosition += 4;
		
			return B_BENDIAN_TO_HOST_INT32(value);
		} else {
			_QTdebugprint("32: TRIED TO READ BEYOND END OF HEADER (hdrPos %ld, size %d)\n",
					   (ulong)(fHeaderPosition - fHeaderBuffer), fMoovSize);
		}
	}
	
	return 0;
}

uint16 QTAtomHandler::ReadMSB16()
{
	if (fDecompressedBuffer) {
		if ((size_t)(fDecompressedPosition - fDecompressedBuffer) < fDecompressedSize) {
			uint16 value = *((uint16 *)fDecompressedPosition);
			fDecompressedPosition += 2;
			
			return B_BENDIAN_TO_HOST_INT16(value);
		} else {
			_QTdebugprint("16: TRIED TO READ BEYOND END OF HEADER (hdrPos %ld, size %d)\n",
					   (ulong)(fHeaderPosition - fHeaderBuffer), fMoovSize);
		}
	} else {
		if ((size_t)(fHeaderPosition - fHeaderBuffer) < fMoovSize) {
			uint16		value = *((uint16 *)fHeaderPosition);
			fHeaderPosition += 2;
			
			return B_BENDIAN_TO_HOST_INT16(value);
		} else {
			_QTdebugprint("16: TRIED TO READ BEYOND END OF DECOMPRESSED HEADER (hdrPos %ld, size %d)\n",
					   (ulong)(fHeaderPosition - fHeaderBuffer), fMoovSize);
		}
	}
	
	return 0;
}

uint8 QTAtomHandler::ReadMSB8()
{
	if (fDecompressedBuffer) {
		if ((size_t)(fDecompressedPosition - fDecompressedBuffer) < fDecompressedSize) {
			uint8 value =  *((uint8 *)fDecompressedPosition);
			fDecompressedPosition += 1;
			return value;
		} else {
			_QTdebugprint("8: TRIED TO READ BEYOND END OF DECOMPRESSED HEADER (hdrPos %ld, size %d)\n",
					   (ulong)(fHeaderPosition - fHeaderBuffer), fMoovSize);
		}
	} else {
		if ((size_t)(fHeaderPosition - fHeaderBuffer) < fMoovSize) {
			uint8		value =  *((uint8 *)fHeaderPosition);
			fHeaderPosition += 1;
			return value;
		} else {
			_QTdebugprint("8: TRIED TO READ BEYOND END OF HEADER (hdrPos %ld, size %d)\n",
					   (ulong)(fHeaderPosition - fHeaderBuffer), fMoovSize);
		}
	}
	
	return 0;
}

void QTAtomHandler::RewindStream(size_t bytes)
{
	if (fDecompressedBuffer)
		fDecompressedPosition -= bytes;
	else
		fHeaderPosition -= bytes;
}

void QTAtomHandler::AdvanceStream(size_t bytes)
{
	if (fDecompressedBuffer)
		fDecompressedPosition += bytes;
	else
		fHeaderPosition += bytes;
}
