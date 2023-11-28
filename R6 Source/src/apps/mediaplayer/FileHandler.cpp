#include <Debug.h>
#include <sys/stat.h>
#include "URL.h"
#include "FileHandler.h"

StreamHandler* FileHandler::InstantiateFileStream(const char *)
{
	return new FileHandler;
}

FileHandler::FileHandler()
{
	PRINT(("FileHandler instantiated\n"));
}

status_t FileHandler::SetTo(const URL &url, BString &outError)
{
	status_t err = fFile.SetTo(url.GetPath(), O_RDONLY);
	if (err < B_OK) {
		outError = strerror(err);
		return err;
	}
	
	struct stat st;
	fFile.GetStat(&st);
	if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)) {
		outError = "This is not a file\n";
		return -1;
	}
		
	fLength = fFile.Seek(0L, SEEK_END);
	fFile.Seek(0L, SEEK_SET);
	fRawDataRate = 0;
	fLastRead = system_time();
	return B_OK;
}

status_t FileHandler::Unset()
{
	fFile.Unset();
	return B_OK;
}

ssize_t FileHandler::ReadAt(off_t pos, void *buffer, size_t size)
{
	ssize_t sizeRead = fFile.ReadAt(pos, buffer, size);

	if (sizeRead >= 0x10000) {
		// Note that we don't count reads that are less than 64k, because
		// they go through the block cache and are really fast.
		// The extractor does a lot of these when it is sniffing, and
		// it throws the numbers off.
		bigtime_t now = system_time();
		double currentSample = (double) sizeRead / ((double) (now - fLastRead + 1) /
			1000000.0);
		fRawDataRate = (double) fRawDataRate / 2 + currentSample / 2;
		fLastRead = now;
	}

	return sizeRead;
}

bool FileHandler::IsDataAvailable(off_t, size_t)
{
	return true;
}

off_t FileHandler::Seek(off_t position, uint32 seek_mode)
{
	return fFile.Seek(position, seek_mode);
}

off_t FileHandler::Position() const
{
	return fFile.Position();
}

off_t FileHandler::GetDownloadedSize() const
{
	return GetLength();
}

size_t FileHandler::GetLength() const
{
	return fLength;
}

void FileHandler::GetStats(player_stats *stats)
{
	stats->raw_data_rate = (int) fRawDataRate;
}


