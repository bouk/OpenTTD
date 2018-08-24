#include "../../stdafx.h"
#include "../../openttd.h"
#include "../../debug.h"
#include "../../fileio_func.h"
#include "../../gfx_func.h"
#include "../../fios.h"
#include "../../string_func.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <threads.h>
#include <switch.h>


void CSleep(int milliseconds)
{
	struct timespec t = {.tv_sec=milliseconds / 1000, .tv_nsec=(milliseconds % 1000) * 1000000};
	thrd_sleep(&t, NULL);
}


bool FiosIsRoot(const char *path)
{
	return path[1] == '\0';
}


void FiosGetDrives(FileList &file_list)
{
	// TODO
}


bool FiosGetDiskFreeSpace(const char *path, uint64 *tot)
{
	// TODO
	*tot = 0;
	return true;
}

bool FiosIsValidFile(const char *path, const struct dirent *ent, struct stat *sb)
{
	char filename[MAX_PATH];
	int res = seprintf(filename, lastof(filename), "%s%s", path, ent->d_name);

	/* Could we fully concatenate the path and filename? */
	if (res >= (int)lengthof(filename) || res < 0) return false;

	return stat(filename, sb) == 0;
}

bool FiosIsHiddenFile(const struct dirent *ent)
{
	return ent->d_name[0] == '.';
}

void ShowInfo(const char *buf)
{
	// TODO
	DEBUG(console, 0, "INFO: %s", buf);
}

void ShowOSErrorBox(const char *buf, bool system)
{
	// TODO
	DEBUG(console, 0, "ERROR: %s", buf);
	socketExit();
}

bool GetClipboardContents(char *buffer, const char *last)
{
	return false;
}

uint GetCPUCoreCount()
{
	// TODO
	return 1;
}

void OSOpenBrowser(const char *url)
{
	// TODO
}

const char *FS2OTTD(const char *name) {return name;}
const char *OTTD2FS(const char *name) {return name;}

int main(int argc, char **argv) {
	socketInitializeDefault();
	nxlinkStdio();
	SetDebugString("9,grf=0,sprite=0,desync=0");

	int ret = openttd_main(1, argv);
	printf("Done. ret=%d\n", ret);
	socketExit();
	return ret;
}
