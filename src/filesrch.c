













#include <stdio.h>
#include <errno.h>
#ifdef __GNUC__
#include <dirent.h>
#endif
#ifdef _WIN32
#define IGNORE_SYMLINKS

#define RPC_NO_WINDOWS_H
#include <windows.h>
#endif
#include <sys/stat.h>
#ifndef S_ISLNK
#define IGNORE_SYMLINKS
#endif

#ifndef IGNORE_SYMLINKS
#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#endif
#include <string.h>

#include "filesrch.h"
#include "netcode/d_netfil.h"
#include "m_misc.h"
#include "z_zone.h"
#include "m_menu.h"  
#include "w_wad.h"

#if defined (_WIN32) && defined (_MSC_VER)

#include <io.h>
#include <tchar.h>

#define SUFFIX	"*"
#define	SLASH	"\\"
#define	S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES	((DWORD)-1)
#endif

struct dirent
{
	long		d_ino;		 
	unsigned short	d_reclen;	 
	unsigned short	d_namlen;	 
	char		d_name[FILENAME_MAX];  
};

  
                                                                       
                                                       
                                                          
   
typedef struct
{
	 
	struct _finddata_t	dd_dta;

	                                                                 
                                                                   
              
	struct dirent		dd_dir;

	 
#if _MSC_VER > 1200
	intptr_t    dd_handle;
#else
	long        dd_handle;
#endif

	  
                     
                                                             
                     
                                            
    
	int			dd_stat;

	 
	CHAR			dd_name[1];
} DIR;

  
          
  
                                                                        
                         
   

DIR *
opendir (const CHAR *szPath)
{
  DIR *nd;
  DWORD rc;
  CHAR szFullPath[MAX_PATH];

  errno = 0;

  if (!szPath)
    {
      errno = EFAULT;
      return (DIR *) 0;
    }

  if (szPath[0] == '\0')
    {
      errno = ENOTDIR;
      return (DIR *) 0;
    }

   
  rc = GetFileAttributesA(szPath);
  if (rc == INVALID_FILE_ATTRIBUTES)
    {
       
      errno = ENOENT;
      return (DIR *) 0;
    }
  if (!(rc & FILE_ATTRIBUTE_DIRECTORY))
    {
       
      errno = ENOTDIR;
      return (DIR *) 0;
    }

   
  _fullpath (szFullPath, szPath, MAX_PATH);

                                                                  
                             
  nd = (DIR *) malloc (sizeof (DIR) + (strlen(szFullPath) + strlen (SLASH) +
			 strlen(SUFFIX) + 1) * sizeof (CHAR));

  if (!nd)
    {
       
      errno = ENOMEM;
      return (DIR *) 0;
    }

   
  strcpy (nd->dd_name, szFullPath);

   
  if (nd->dd_name[0] != '\0' &&
      nd->dd_name[strlen (nd->dd_name) - 1] != '/' &&
      nd->dd_name[strlen (nd->dd_name) - 1] != '\\')
    {
      strcat (nd->dd_name, SLASH);
    }

   
  strcat (nd->dd_name, SUFFIX);

                                                                     
                                 
  nd->dd_handle = -1;

   
  nd->dd_stat = 0;

                                                                      
                                                                 
                              
  nd->dd_dir.d_ino = 0;
  nd->dd_dir.d_reclen = 0;
  nd->dd_dir.d_namlen = 0;
  ZeroMemory(nd->dd_dir.d_name, FILENAME_MAX);

  return nd;
}

  
          
  
                                                                            
                               
   
struct dirent *
readdir (DIR * dirp)
{
  errno = 0;

   
  if (!dirp)
    {
      errno = EFAULT;
      return (struct dirent *) 0;
    }

  if (dirp->dd_stat < 0)
    {
                                                            
                                                      
      return (struct dirent *) 0;
    }
  else if (dirp->dd_stat == 0)
    {
       
       
      dirp->dd_handle = _findfirst (dirp->dd_name, &(dirp->dd_dta));

	  if (dirp->dd_handle == -1)
	{
	                                             
                   
	  dirp->dd_stat = -1;
	}
      else
	{
	  dirp->dd_stat = 1;
	}
    }
  else
    {
       
      if (_findnext (dirp->dd_handle, &(dirp->dd_dta)))
	{
	                                           
                                                    
                   
	  DWORD winerr = GetLastError();
	  if (winerr == ERROR_NO_MORE_FILES)
	    errno = 0;
	  _findclose (dirp->dd_handle);
	  dirp->dd_handle = -1;
	  dirp->dd_stat = -1;
	}
      else
	{
	                                              
                
	  dirp->dd_stat++;
	}
    }

  if (dirp->dd_stat > 0)
    {
                                                                
                                                                 
                      
      dirp->dd_dir.d_namlen = (unsigned short)strlen (dirp->dd_dta.name);
      strcpy (dirp->dd_dir.d_name, dirp->dd_dta.name);
      return &dirp->dd_dir;
    }

  return (struct dirent *) 0;
}

  
            
  
                                                   
   
int
rewinddir (DIR * dirp)
{
  errno = 0;

   
  if (!dirp)
    {
      errno = EFAULT;
      return -1;
    }

  dirp->dd_stat = 0;

  return 0;
}

  
           
  
                                           
   
int
closedir (DIR * dirp)
{
  int rc;

  errno = 0;
  rc = 0;

  if (!dirp)
    {
      errno = EFAULT;
      return -1;
    }

  if (dirp->dd_handle != -1)
    {
      rc = _findclose (dirp->dd_handle);
    }

   
  free (dirp);

  return rc;
}
#endif





FILE *fopenfile(const char *path, const char *mode)
{
	FILE *h = fopen(path, mode);

	if (h != NULL)
	{
		struct stat st;
		int eno;

		if (fstat(fileno(h), &st) == -1)
		{
			eno = errno;
		}
		else if (!S_ISREG(st.st_mode))
		{
			eno = EACCES;  
		}
		else
		{
			return h;  
		}

		fclose(h);
		errno = eno;
	}

	return NULL;
}

static CV_PossibleValue_t addons_cons_t[] = {{0, "Default"},
#if 1
												{1, "HOME"}, {2, "SRB2"},
#endif
													{3, "CUSTOM"}, {0, NULL}};

consvar_t cv_addons_option = CVAR_INIT ("addons_option", "Default", CV_SAVE|CV_CALL, addons_cons_t, Addons_option_Onchange);
consvar_t cv_addons_folder = CVAR_INIT ("addons_folder", "", CV_SAVE, NULL, NULL);

static CV_PossibleValue_t addons_md5_cons_t[] = {{0, "Name"}, {1, "Contents"}, {0, NULL}};
consvar_t cv_addons_md5 = CVAR_INIT ("addons_md5", "Name", CV_SAVE, addons_md5_cons_t, NULL);

consvar_t cv_addons_showall = CVAR_INIT ("addons_showall", "No", CV_SAVE, CV_YesNo, NULL);

consvar_t cv_addons_search_case = CVAR_INIT ("addons_search_case", "No", CV_SAVE, CV_YesNo, NULL);

static CV_PossibleValue_t addons_search_type_cons_t[] = {{0, "Start"}, {1, "Anywhere"}, {0, NULL}};
consvar_t cv_addons_search_type = CVAR_INIT ("addons_search_type", "Anywhere", CV_SAVE, addons_search_type_cons_t, NULL);

char menupath[1024];
size_t menupathindex[menudepth];
size_t menudepthleft = menudepth;

char menusearch[MAXSTRINGLENGTH+1];

char **dirmenu, **coredirmenu;  
size_t sizedirmenu, sizecoredirmenu;  
size_t dir_on[menudepth];
UINT8 refreshdirmenu = 0;
char *refreshdirname = NULL;

#define dirpathlen 1024
#define maxdirdepth 48

#define isuptree(dirent) ((dirent)[0]=='.' && ((dirent)[1]=='\0' || ((dirent)[1]=='.' && (dirent)[2]=='\0')))

filestatus_t filesearch(char *filename, const char *startpath, const UINT8 *wantedmd5sum, boolean completepath, int maxsearchdepth)
{
	filestatus_t retval = FS_NOTFOUND;
	DIR **dirhandle;
	struct dirent *dent;
	struct stat fsstat;
	int found = 0;
	char *searchname = strdup(filename);
	int depthleft = maxsearchdepth;
	char searchpath[1024];
	size_t *searchpathindex;

	dirhandle = (DIR**) malloc(maxsearchdepth * sizeof (DIR*));
	searchpathindex = (size_t *) malloc(maxsearchdepth * sizeof (size_t));

	strcpy(searchpath,startpath);
	searchpathindex[--depthleft] = strlen(searchpath) + 1;

	dirhandle[depthleft] = opendir(searchpath);

	if (dirhandle[depthleft] == NULL)
	{
		free(searchname);
		free(dirhandle);
		free(searchpathindex);
		return FS_NOTFOUND;
	}

	if (searchpath[searchpathindex[depthleft]-2] != PATHSEP[0])
	{
		searchpath[searchpathindex[depthleft]-1] = PATHSEP[0];
		searchpath[searchpathindex[depthleft]] = 0;
	}
	else
		searchpathindex[depthleft]--;

	while ((!found) && (depthleft < maxsearchdepth))
	{
		searchpath[searchpathindex[depthleft]]=0;
		dent = readdir(dirhandle[depthleft]);

		if (!dent)
		{
			closedir(dirhandle[depthleft++]);
			continue;
		}

		if (isuptree(dent->d_name))
		{

			continue;
		}


		strcpy(&searchpath[searchpathindex[depthleft]],dent->d_name);

#if defined(__linux__) || defined(__FreeBSD__)
		if (dent->d_type == DT_UNKNOWN || dent->d_type == DT_LNK)
			if (stat(searchpath,&fsstat) == 0 && S_ISDIR(fsstat.st_mode))
				dent->d_type = DT_DIR;


		if (dent->d_type == DT_DIR && depthleft)
#else
		if (stat(searchpath,&fsstat) < 0)  
			;  
		else if (S_ISDIR(fsstat.st_mode) && depthleft)
#endif
		{
			searchpathindex[--depthleft] = strlen(searchpath) + 1;
			dirhandle[depthleft] = opendir(searchpath);
			if (!dirhandle[depthleft])
			{


					depthleft++;
			}

			searchpath[searchpathindex[depthleft]-1]=PATHSEP[0];
			searchpath[searchpathindex[depthleft]]=0;
		}
		else if (!strcasecmp(searchname, dent->d_name))
		{
#ifndef IGNORE_SYMLINKS
			struct stat statbuf;
#endif
			switch (checkfilemd5(searchpath, wantedmd5sum))
			{
				case FS_FOUND:
					if (completepath)
						strcpy(filename,searchpath);
					else
						strcpy(filename,dent->d_name);
#ifndef IGNORE_SYMLINKS
					if (lstat(filename, &statbuf) != -1)
					{
						if (S_ISLNK(statbuf.st_mode))
						{
							char *tempbuf = realpath(filename, NULL);
							if (!tempbuf)
								I_Error("Error parsing link %s: %s", filename, strerror(errno));
							strncpy(filename, tempbuf, MAX_WADPATH);
							free(tempbuf);
						}
					}
#endif
					retval = FS_FOUND;
					found = 1;
					break;
				case FS_MD5SUMBAD:
					retval = FS_MD5SUMBAD;
					break;
				default:  
					break;
			}
		}
	}

	for (; depthleft < maxsearchdepth; closedir(dirhandle[depthleft++]));

	free(searchname);
	free(searchpathindex);
	free(dirhandle);

	return retval;
}

#ifndef AVOID_ERRNO
int direrror = 0;
#endif




INT32 pathisdirectory(const char *path)
{
	struct stat fsstat;

	if (stat(path, &fsstat) < 0)
	{
#ifndef AVOID_ERRNO
		direrror = errno;
#endif
		return -1;
	}
	else if (S_ISDIR(fsstat.st_mode))
		return 1;

	return 0;
}



INT32 concatpaths(const char *path, const char *startpath)
{
	char dirpath[dirpathlen];
	DIR *dirhandle;
	INT32 stat;

	if (startpath)
	{
		char basepath[dirpathlen];

		snprintf(basepath, sizeof basepath, "%s" PATHSEP, startpath);
		snprintf(dirpath, sizeof dirpath, "%s%s", basepath, path);


		stat = samepaths(basepath, dirpath);

		if (stat == 1)
			return 0;
		else if (stat < 0)
			return -1;
	}
	else
		snprintf(dirpath, sizeof dirpath, "%s", path);



	stat = pathisdirectory(dirpath);
	if (stat == 0)
		return 0;
	else if (stat < 0)
	{

		if (direrror == ENOENT)
			return 0;

		return -1;
	}



	dirhandle = opendir(dirpath);
	if (dirhandle == NULL)
		return 0;
	else
		closedir(dirhandle);

	return 1;
}





INT32 samepaths(const char *path1, const char *path2)
{
	struct stat stat1;
	struct stat stat2;

	if (stat(path1, &stat1) < 0)
	{
#ifndef AVOID_ERRNO
		direrror = errno;
#endif
		return -1;
	}
	if (stat(path2, &stat2) < 0)
	{
#ifndef AVOID_ERRNO
		direrror = errno;
#endif
		return -2;
	}

	if (stat1.st_dev == stat2.st_dev)
	{
#if !defined(_WIN32)
		if (stat1.st_ino == stat2.st_ino)
			return 1;
#else

		HANDLE file1 = CreateFileA(path1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		HANDLE file2 = CreateFileA(path2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		BY_HANDLE_FILE_INFORMATION file1info, file2info;

		if (file1 == INVALID_HANDLE_VALUE)
		{
#ifndef AVOID_ERRNO
			direrror = ENOENT;
#endif
			return -1;
		}
		else if (file2 == INVALID_HANDLE_VALUE)
		{
			CloseHandle(file1);
#ifndef AVOID_ERRNO
			direrror = ENOENT;
#endif
			return -2;
		}




		if (!GetFileInformationByHandle(file1, &file1info))
		{
			CloseHandle(file1);
			CloseHandle(file2);
#ifndef AVOID_ERRNO
			direrror = EIO;
#endif
			return -1;
		}
		else if (!GetFileInformationByHandle(file2, &file2info))
		{
			CloseHandle(file1);
			CloseHandle(file2);
#ifndef AVOID_ERRNO
			direrror = EIO;
#endif
			return -2;
		}

		int status = 0;

		if (file1info.dwVolumeSerialNumber == file2info.dwVolumeSerialNumber
		&& file1info.nFileIndexLow == file2info.nFileIndexLow
		&& file1info.nFileIndexHigh == file2info.nFileIndexHigh)
		{
			status = 1;
		}

		CloseHandle(file1);
		CloseHandle(file2);

		return status;
#endif
	}

	return 0;
}





static void initdirpath(char *dirpath, size_t *dirpathindex, int depthleft)
{
	dirpathindex[depthleft] = strlen(dirpath) + 1;

	if (dirpath[dirpathindex[depthleft]-2] != PATHSEP[0])
	{
		dirpath[dirpathindex[depthleft]-1] = PATHSEP[0];
		dirpath[dirpathindex[depthleft]] = 0;
	}
	else
		dirpathindex[depthleft]--;
}


static int lumpnamecompare(const void *A, const void *B)
{
	const lumpinfo_t *pA = A;
	const lumpinfo_t *pB = B;
	return strcmp((pA->fullname), (pB->fullname));

}

lumpinfo_t *getdirectoryfiles(const char *path, UINT16 *nlmp, UINT16 *nfolders)
{
	DIR **dirhandle;
	struct dirent *dent;
	struct stat fsstat;

	int rootdir = (maxdirdepth - 1);
	int depthleft = rootdir;

	char dirpath[dirpathlen];
	size_t *dirpathindex;

	lumpinfo_t *lumpinfo, *lump_p;
	UINT16 i = 0, numlumps = 0;

	boolean failure = false;

	dirhandle = (DIR **)malloc(maxdirdepth * sizeof (DIR*));
	dirpathindex = (size_t *)malloc(maxdirdepth * sizeof(size_t));


	strlcpy(dirpath, path, dirpathlen);
	dirhandle[depthleft] = opendir(dirpath);

	if (dirhandle[depthleft] == NULL)
	{
		free(dirhandle);
		free(dirpathindex);
		return NULL;
	}

	initdirpath(dirpath, dirpathindex, depthleft);
	(*nfolders) = 0;


	while (depthleft < maxdirdepth)
	{
		dirpath[dirpathindex[depthleft]] = 0;
		dent = readdir(dirhandle[depthleft]);

		if (!dent)
		{
			if (depthleft != rootdir)  
				closedir(dirhandle[depthleft]);
			depthleft++;
			continue;
		}
		else if (isuptree(dent->d_name))
			continue;

		strcpy(&dirpath[dirpathindex[depthleft]], dent->d_name);

		if (stat(dirpath, &fsstat) < 0)
			;
		else if (S_ISDIR(fsstat.st_mode) && depthleft)
		{
			dirpathindex[--depthleft] = strlen(dirpath) + 1;
			dirhandle[depthleft] = opendir(dirpath);

			if (dirhandle[depthleft])
				(*nfolders)++;
			else
				depthleft++;

			dirpath[dirpathindex[depthleft]-1] = '/';
			dirpath[dirpathindex[depthleft]] = 0;
		}
		else
			numlumps++;


		if (numlumps == UINT16_MAX)
		{
			(*nlmp) = UINT16_MAX;
			failure = true;
			break;
		}
	}


	if (!numlumps)
	{
		(*nlmp) = 0;
		failure = true;
	}


	if (failure)
	{
		for (; depthleft < maxdirdepth; closedir(dirhandle[depthleft++]));
		free(dirpathindex);
		free(dirhandle);
		return NULL;
	}




	rewinddir(dirhandle[rootdir]);
	depthleft = rootdir;

	strlcpy(dirpath, path, dirpathlen);
	initdirpath(dirpath, dirpathindex, depthleft);

	lump_p = lumpinfo = Z_Calloc(numlumps * sizeof(lumpinfo_t), PU_STATIC, NULL);

	while (depthleft < maxdirdepth)
	{
		char *fullname, *trimname;

		dirpath[dirpathindex[depthleft]] = 0;
		dent = readdir(dirhandle[depthleft]);

		if (!dent)
		{
			closedir(dirhandle[depthleft++]);
			continue;
		}
		else if (isuptree(dent->d_name))
			continue;

		strcpy(&dirpath[dirpathindex[depthleft]], dent->d_name);

		if (stat(dirpath, &fsstat) < 0)
			continue;
		else if (S_ISDIR(fsstat.st_mode) && depthleft)
		{
			dirpathindex[--depthleft] = strlen(dirpath) + 1;
			dirhandle[depthleft] = opendir(dirpath);

			if (dirhandle[depthleft])
			{
				dirpath[dirpathindex[depthleft]-1] = '/';
				dirpath[dirpathindex[depthleft]] = 0;
			}
			else
				depthleft++;

			continue;
		}

		lump_p->diskpath = Z_StrDup(dirpath);  
		lump_p->compression = CM_NOCOMPRESSION;  
		lump_p->size = lump_p->disksize = fsstat.st_size;


		fullname = lump_p->diskpath;
		if (strstr(fullname, path))
			fullname += strlen(path) + 1;


		trimname = strrchr(fullname, '/');
		if (trimname)
			trimname++;
		else
			trimname = fullname;

		if (trimname[0])
		{
			char *dotpos = strrchr(trimname, '.');
			if (dotpos == NULL)
				dotpos = fullname + strlen(fullname);

			strncpy(lump_p->name, trimname, min(8, dotpos - trimname));


			lump_p->longname = Z_Calloc(dotpos - trimname + 1, PU_STATIC, NULL);
			strlcpy(lump_p->longname, trimname, dotpos - trimname + 1);
		}
		else
			lump_p->longname = Z_Calloc(1, PU_STATIC, NULL);



		lump_p->fullname = Z_StrDup(fullname);
		lump_p->hash = quickncasehash(lump_p->name, 8);

		lump_p++;
		i++;

		if (i > numlumps || i == (UINT16_MAX-1))
		{
			for (; depthleft < maxdirdepth; closedir(dirhandle[depthleft++]));  
			break;
		}
	}

	free(dirpathindex);
	free(dirhandle);


	qsort (lumpinfo, numlumps, sizeof(lumpinfo_t), lumpnamecompare);

	(*nlmp) = numlumps;
	return lumpinfo;
}





char exttable[NUM_EXT_TABLE][7] = {  
	"\5.txt", "\5.cfg",  
	"\5.wad",
#ifdef USE_KART
	"\6.kart",
#endif
	"\5.pk3", "\5.soc", "\5.lua"};  

static char (*filenamebuf)[MAX_WADPATH];

static boolean filemenucmp(char *haystack, char *needle)
{
	static char localhaystack[128];
	strlcpy(localhaystack, haystack, 128);
	if (!cv_addons_search_case.value)
		strupr(localhaystack);
	if (cv_addons_search_type.value)
		return (strstr(localhaystack, needle) != 0);
	return (!strncmp(localhaystack, needle, menusearch[0]));
}

void closefilemenu(boolean validsize)
{

	if (dirmenu)
	{
		if (dirmenu != coredirmenu)
		{
			if (dirmenu[0] && ((UINT8)(dirmenu[0][DIR_TYPE]) == EXT_NORESULTS))
			{
				Z_Free(dirmenu[0]);
				dirmenu[0] = NULL;
			}
			Z_Free(dirmenu);
		}
		dirmenu = NULL;
		sizedirmenu = 0;
	}

	if (coredirmenu)
	{

		if (validsize)
		{
			for (; sizecoredirmenu > 0; sizecoredirmenu--)
			{
				Z_Free(coredirmenu[sizecoredirmenu-1]);
				coredirmenu[sizecoredirmenu-1] = NULL;
			}
		}
		else
			sizecoredirmenu = 0;

		Z_Free(coredirmenu);
		coredirmenu = NULL;
	}

	if (refreshdirname)
		Z_Free(refreshdirname);
	refreshdirname = NULL;
}

void searchfilemenu(char *tempname)
{
	size_t i, first;
	char localmenusearch[MAXSTRINGLENGTH] = "";

	if (dirmenu)
	{
		if (dirmenu != coredirmenu)
		{
			if (dirmenu[0] && ((UINT8)(dirmenu[0][DIR_TYPE]) == EXT_NORESULTS))
			{
				Z_Free(dirmenu[0]);
				dirmenu[0] = NULL;
			}

		}
		else
			dirmenu = NULL;
	}

	first = (((UINT8)(coredirmenu[0][DIR_TYPE]) == EXT_UP) ? 1 : 0);  

	if (!menusearch[0])
	{
		if (dirmenu)
			Z_Free(dirmenu);
		dirmenu = coredirmenu;
		sizedirmenu = sizecoredirmenu;

		if (tempname)
		{
			for (i = first; i < sizedirmenu; i++)
			{
				if (!strcmp(dirmenu[i]+DIR_STRING, tempname))
				{
					dir_on[menudepthleft] = i;
					break;
				}
			}

			if (i == sizedirmenu)
				dir_on[menudepthleft] = first;

			Z_Free(tempname);
		}

		return;
	}

	strcpy(localmenusearch, menusearch+1);
	if (!cv_addons_search_case.value)
		strupr(localmenusearch);

	sizedirmenu = 0;
	for (i = first; i < sizecoredirmenu; i++)
	{
		if (filemenucmp(coredirmenu[i]+DIR_STRING, localmenusearch))
			sizedirmenu++;
	}

	if (!sizedirmenu)  
	{
		if ((!(dirmenu = Z_Realloc(dirmenu, sizeof(char *), PU_STATIC, NULL)))
			|| !(dirmenu[0] = Z_StrDup(va("%c\13No results...", EXT_NORESULTS))))
				I_Error("searchfilemenu(): could not create \"No results...\".");
		sizedirmenu = 1;
		dir_on[menudepthleft] = 0;
		if (tempname)
			Z_Free(tempname);
		return;
	}

	if (!(dirmenu = Z_Realloc(dirmenu, sizedirmenu*sizeof(char *), PU_STATIC, NULL)))
		I_Error("searchfilemenu(): could not reallocate dirmenu.");

	sizedirmenu = 0;
	for (i = first; i < sizecoredirmenu; i++)
	{
		if (filemenucmp(coredirmenu[i]+DIR_STRING, localmenusearch))
		{
			if (tempname && !strcmp(coredirmenu[i]+DIR_STRING, tempname))
			{
				dir_on[menudepthleft] = sizedirmenu;
				Z_Free(tempname);
				tempname = NULL;
			}
			dirmenu[sizedirmenu++] = coredirmenu[i];  
		}
	}

	if (tempname)
	{
		dir_on[menudepthleft] = 0;  
		Z_Free(tempname);
	}
}

boolean preparefilemenu(boolean samedepth)
{
	DIR *dirhandle;
	struct dirent *dent;
	struct stat fsstat;
	size_t pos = 0, folderpos = 0, numfolders = 0;
	char *tempname = NULL;

	if (samedepth)
	{
		if (dirmenu && dirmenu[dir_on[menudepthleft]])
			tempname = Z_StrDup(dirmenu[dir_on[menudepthleft]]+DIR_STRING);  
	}
	else
		menusearch[0] = menusearch[1] = 0;  

	if (!(dirhandle = opendir(menupath)))  
	{
		closefilemenu(true);
		return false;
	}

	for (; sizecoredirmenu > 0; sizecoredirmenu--)  
	{
		Z_Free(coredirmenu[sizecoredirmenu-1]);
		coredirmenu[sizecoredirmenu-1] = NULL;
	}

	while (true)
	{
		menupath[menupathindex[menudepthleft]] = 0;
		dent = readdir(dirhandle);

		if (!dent)
			break;
		else if (isuptree(dent->d_name))
			continue;  

		strcpy(&menupath[menupathindex[menudepthleft]],dent->d_name);

		if (stat(menupath,&fsstat) < 0)  
			;  
		else  
		{
			if (!S_ISDIR(fsstat.st_mode))  
			{
				if (!cv_addons_showall.value)
				{
					size_t len = strlen(dent->d_name)+1;
					UINT8 ext;
					for (ext = 0; ext < NUM_EXT_TABLE; ext++)
						if (!strcasecmp(exttable[ext]+1, dent->d_name+len-(exttable[ext][0]))) break;  
					if (ext == NUM_EXT_TABLE) continue;  
				}
			}
			else  
				numfolders++;

			sizecoredirmenu++;
		}
	}

	if (!sizecoredirmenu)
	{
		closedir(dirhandle);
		closefilemenu(false);
		if (tempname)
			Z_Free(tempname);
		return false;
	}

	if (menudepthleft != menudepth-1)  
	{
		sizecoredirmenu++;
		numfolders++;
		folderpos++;
	}

	if (dirmenu && dirmenu == coredirmenu)
		dirmenu = NULL;

	if (!(coredirmenu = Z_Realloc(coredirmenu, sizecoredirmenu*sizeof(char *), PU_STATIC, NULL)))
	{
		closedir(dirhandle);  
		I_Error("preparefilemenu(): could not reallocate coredirmenu.");
	}

	rewinddir(dirhandle);

	while ((pos+folderpos) < sizecoredirmenu)
	{
		menupath[menupathindex[menudepthleft]] = 0;
		dent = readdir(dirhandle);

		if (!dent)
			break;
		else if (isuptree(dent->d_name))
			continue;  

		strcpy(&menupath[menupathindex[menudepthleft]],dent->d_name);

		if (stat(menupath,&fsstat) < 0)  
			;  
		else  
		{
			char *temp;
			size_t len = strlen(dent->d_name)+1;
			UINT8 ext = EXT_FOLDER;
			UINT8 folder;

			if (!S_ISDIR(fsstat.st_mode))  
			{
				if (!((numfolders+pos) < sizecoredirmenu)) continue;  
				for (; ext < NUM_EXT_TABLE; ext++)
					if (!strcasecmp(exttable[ext]+1, dent->d_name+len-(exttable[ext][0]))) break;  
				if (ext == NUM_EXT_TABLE && !cv_addons_showall.value) continue;  
				ext += EXT_START;  

				if (ext >= EXT_LOADSTART)
				{
					size_t i;

					if (filenamebuf == NULL)
						filenamebuf = calloc(numwadfiles, sizeof(char) * MAX_WADPATH);

					for (i = 0; i < numwadfiles; i++)
					{
						if (!filenamebuf[i][0])
						{
							strncpy(filenamebuf[i], wadfiles[i]->filename, MAX_WADPATH-1);
							filenamebuf[i][MAX_WADPATH - 1] = '\0';
							nameonly(filenamebuf[i]);
						}

						if (strcmp(dent->d_name, filenamebuf[i]))
							continue;
						if (cv_addons_md5.value && !checkfilemd5(menupath, wadfiles[i]->md5sum))
							continue;

						ext |= EXT_LOADED;
					}
				}
				else if (ext == EXT_TXT)
				{
					if (!strncmp(dent->d_name, "log-", 4) || !strcmp(dent->d_name, "errorlog.txt"))
						ext |= EXT_LOADED;
				}

				if (!strcmp(dent->d_name, configfile))
					ext |= EXT_LOADED;

				folder = 0;
			}
			else  
				len += (folder = 1);

			if (len > 255)
				len = 255;

			if (!(temp = Z_Malloc((len+DIR_STRING+folder) * sizeof (char), PU_STATIC, NULL)))
				I_Error("preparefilemenu(): could not create file entry.");
			temp[DIR_TYPE] = ext;
			temp[DIR_LEN] = (UINT8)(len);
			strlcpy(temp+DIR_STRING, dent->d_name, len);
			if (folder)
			{
				strcpy(temp+len, PATHSEP);
				coredirmenu[folderpos++] = temp;
			}
			else
				coredirmenu[numfolders + pos++] = temp;
		}
	}

	if (filenamebuf)
	{
		free(filenamebuf);
		filenamebuf = NULL;
	}

	closedir(dirhandle);

	if ((menudepthleft != menudepth-1)  
		&& !(coredirmenu[0] = Z_StrDup(va("%c\5UP...", EXT_UP))))
			I_Error("preparefilemenu(): could not create \"UP...\".");

	menupath[menupathindex[menudepthleft]] = 0;
	sizecoredirmenu = (numfolders+pos);  

	if (!sizecoredirmenu)
	{
		dir_on[menudepthleft] = 0;
		closefilemenu(false);
		return false;
	}

	searchfilemenu(tempname);

	return true;
}
