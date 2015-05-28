/*
 * filemanager.c
 *
 *  Created on: 19.05.2015
 *      Author: Nicolaj Hoess
 */

#include "filemanager.h"
#include "hardcoded_programs.h"
#include "../devicemanager/devicemanager.h"
#include "../processmanager/processmanager.h"
#include "../systemapi/includes/system.h"
#include "../filesystem/ff.h"
#include "../driver/sdcard/mmcsd_proto.h"

#define FILE_MANAGER_MAX_PATH_LENGTH	(512)

// #pragma DATA_ALIGN(fatFileSystem, SOC_CACHELINE_SIZE);
static FATFS fatFileSystem;
typedef struct {
	void *dev;
	FATFS *fs;
	boolean_t initDone;
} externalStorageDevice_t;

extern externalStorageDevice_t fat_devices[2];

static DIR 			dir;
static FILINFO 		fileInfo;

static char currentWorkingDirectory[FILE_MANAGER_MAX_PATH_LENGTH];

static void mountFatDevice(unsigned int driveNum, void*);

int FileManagerOpenExecutable(char* name, boolean_t searchInGlobalBinPath, int argc, char** argv, boolean_t blocking, context_t* context)
{
	if (searchInGlobalBinPath)
	{
		// Search in global bin path first (this includes hardcoded user-processes)
		void (*funcPtr)(int, char ** ) = HardCodedProgramsGetProgram(name);
		if (funcPtr != NULL)
		{
			process_t* proc = ProcessManagerStartProcess(name, funcPtr, blocking, context);
			return FILE_MANAGER_OK;
		}
	}

	// TODO Search in current working directory

	return FILE_MANAGER_NOT_FOUND;
}

int FileManagerInit(device_t device) {
	if (DeviceManagerInitDevice(device) != DRIVER_OK)
	{
		return FILE_MANAGER_NO_DEVICE_FOUND;
	}
	DeviceManagerOpen(device);

	int length = sizeof(mmcsdCardInfo);
	char* sdCard = (char*) malloc(length);

	if (DeviceManagerRead(device, sdCard, length) != DRIVER_OK)
	{
		return FILE_MANAGER_NO_DEVICE_FOUND;
	}

	mmcsdCardInfo* card = (mmcsdCardInfo*)(sdCard);

	mountFatDevice(0, card);

	f_mount(0, &fatFileSystem);
	fat_devices[0].dev = card;
	fat_devices[0].fs = &fatFileSystem;
	fat_devices[0].initDone = false;

	strncpy(currentWorkingDirectory, FILE_MANAGER_ROOT_PATH, FILE_MANAGER_MAX_PATH_LENGTH);

	return FILE_MANAGER_OK;
}

int FileManagerListDirectoryContent(const char* name, entryType_t* buf, int length) {
	volatile FRESULT fresult;

	// Open directory
	if (f_opendir(&dir, currentWorkingDirectory))
	{
		return FILE_MANAGER_NOT_FOUND;
	}

	while (1) {
		if (f_readdir(&dir, &fileInfo) != FR_OK)
		{
			// Error occured while reading: Doing cleanup of the incomplete results and return
			memset(buf, 0, sizeof(entryType_t) * length);
			return FILE_MANAGER_NOT_FOUND;
		}

		// If the file name is blank, then this is the end of the listing.
		if (!fileInfo.fname[0]) { break; }

		// Print the entry information on a single line with formatting to show
		// the attributes, date, time, size, and name.
		printf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s\n",
				(fileInfo.fattrib & AM_DIR) ? 'D' : '-',
				(fileInfo.fattrib & AM_RDO) ? 'R' : '-',
				(fileInfo.fattrib & AM_HID) ? 'H' : '-',
				(fileInfo.fattrib & AM_SYS) ? 'S' : '-',
				(fileInfo.fattrib & AM_ARC) ? 'A' : '-',
				(fileInfo.fdate >> 9) + 1980,
				(fileInfo.fdate >> 5) & 15, fileInfo.fdate & 31,
				(fileInfo.ftime >> 11),
				(fileInfo.ftime >> 5) & 63, fileInfo.fsize,
				fileInfo.fname);
	}

	return FILE_MANAGER_OK;
}

int FileManagerOpenFile(const char* name, int startByte, char* buf, int length) {
	FIL file;
	if (f_open(&file, "/nico.txt", FA_READ) != FR_OK)
	{
		return FILE_MANAGER_NOT_FOUND;
	}
	char buffer[1000];
	unsigned short read = 0;

	if (f_read(&file, &buffer, 1000, &read) != FR_OK)
	{
		memset(buf, 0, sizeof(char) * length);
		return FILE_MANAGER_NOT_FOUND;
	}

	f_close(&file);

	printf("Opened file nico.txt: \n%s\n", buffer);

	return FILE_MANAGER_OK;
}

static void mountFatDevice(unsigned int driveNum, void* ptr) {
	f_mount(driveNum, &fatFileSystem);
	fat_devices[driveNum].dev = ptr;
	fat_devices[driveNum].fs = &fatFileSystem;
	fat_devices[driveNum].initDone = false;
}
