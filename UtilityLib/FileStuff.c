#include	<stdint.h>
#include	<stdbool.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<assert.h>
#include	<dirent.h>
#include	<errno.h>
#include	"utstring.h"


//handy stuff I'm used to from C#
bool	FileStuff_DirExists(const char *szPath)
{
	DIR	*pDir	=opendir(szPath);

	if(pDir == NULL)
	{
		return	false;
	}
	else if(ENOENT == errno)
	{
		return	false;
	}

	closedir(pDir);

	return	true;
}


//this is for those errno.h errors
void	FileStuff_PrintErrno(int err)
{
	if(err == 0)
	{
		return;	//success!
	}

	switch(err)
	{
		case	EACCES:
			printf("EACCESS\n");
			break;
		case	EIO:
			printf("EI EI O (EIO)\n");
			break;
		case	ELOOP:
			printf("ELOOP\n");
			break;
		case	ENAMETOOLONG:
			printf("ENAMETOOLONG\n");
			break;
		case	ENOENT:
			printf("ENOENT\n");
			break;
		case	ENOTDIR:
			printf("ENOTDIR\n");
			break;
		case	EOVERFLOW:
			printf("EOVERFLOW\n");
			break;
		default:
			printf("This: %d happened ¯\\_(ツ)_/¯\n", err);
	}
}
