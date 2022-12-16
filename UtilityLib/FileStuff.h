#include	<stdint.h>
#include	<stdbool.h>


extern bool	FileStuff_DirExists(const char *szPath);

//this is for those errno.h errors
extern void	FileStuff_PrintErrno(int err);
