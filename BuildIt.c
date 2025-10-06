#define NOB_IMPLEMENTATION
#define	NOB_EXPERIMENTAL_DELETE_OLD
#include	"nob.h"
#include	<stdlib.h>


static void	sStandardLibJunk_Add(Nob_Cmd *pCmd)
{
	//stuff all the libraries like
	nob_cmd_append(pCmd, "gcc", "-g", "-O0", "-march=native",
		"-fpic", "-shared",
		"-DCGLM_FORCE_DEPTH_ZERO_TO_ONE",
		"-DCGLM_FORCE_LEFT_HANDED",
		"-I../uthash/src",
		"-I../dxvk-native/include/native/directx",
		"-I../dxvk-native/include/native/windows",
		"-I../dxvk-native/include/native",
		"-I../dxvk-native/include",
		"-I../cglm/include", "-Wall",
		"-Wl,-rpath='$$ORIGIN',--disable-new-dtags");
}

static bool	sbIsC(const char *szFileName)
{
	size_t	len	=strlen(szFileName);
	if(len < 3)
	{
		return	false;
	}

	return	(szFileName[len - 2] == '.'
		&& szFileName[len - 1] == 'c');
}

static bool	sbIsCpp(const char *szFileName)
{
	size_t	len	=strlen(szFileName);
	if(len < 5)
	{
		return	false;
	}

	return	(szFileName[len - 4] == '.'
		&& szFileName[len - 3] == 'c'
		&& szFileName[len - 2] == 'p'
		&& szFileName[len - 1] == 'p');
}

static bool sBuildPhysicsLib(const char *szPath)
{
	Nob_Cmd	libCmd	={0};

	//cd to dir
	if(!nob_set_current_dir(szPath))
	{
		printf("Something went wrong changing to lib dir %s\n", szPath);
		return	false;
	}
	
	sStandardLibJunk_Add(&libCmd);

	//extra stuff needed by jolt things
	nob_cmd_append(&libCmd,
		"-std=gnu++23",
		"-DJPH_PROFILE_ENABLED",
		"-DJPH_DEBUG_RENDERER",
		"-DJPH_OBJECT_STREAM",
		"-I../JoltPhysics");

    Nob_File_Paths	cppFiles	={0};

	if(!nob_read_entire_dir(".", &cppFiles))
	{
		printf("Something went wrong reading dir %s\n", szPath);
		return	false;
	}

	//add all the .cpp files
	for(int i=0;i < cppFiles.count;i++)
	{
		if(sbIsCpp(cppFiles.items[i]))
		{
			nob_cmd_append(&libCmd, cppFiles.items[i]);
		}
	}

	//more args
	nob_cmd_append(&libCmd, "-o");

	//make the wierdly named so file name
	char	tmp[128];
	sprintf(tmp, "../libs/lib%s.so", szPath);
	nob_cmd_append(&libCmd,
		tmp, "../JoltPhysics/Build/Linux_Debug/libJolt.a");

	if(!nob_cmd_run(&libCmd))	return	false;

	//pop back up a dir
	if(!nob_set_current_dir(".."))
	{
		printf("Something went wrong changing to ..\n");
		return	false;
	}

	nob_da_free(cppFiles);
	nob_cmd_free(libCmd);
	
	return	true;
}

static bool	sBuildCLib(const char *szPath)
{
	Nob_Cmd	libCmd	={0};

	//cd to dir
	if(!nob_set_current_dir(szPath))
	{
		printf("Something went wrong changing to lib dir %s\n", szPath);
		return	false;
	}
	
	sStandardLibJunk_Add(&libCmd);

    Nob_File_Paths	cFiles	={0};

	if(!nob_read_entire_dir(".", &cFiles))
	{
		printf("Something went wrong reading dir %s\n", szPath);
		return	false;
	}

	//add all the .c files
	for(int i=0;i < cFiles.count;i++)
	{
//		printf("Filez:%s \n", cFiles.items[i]);
		if(sbIsC(cFiles.items[i]))
		{
			nob_cmd_append(&libCmd, cFiles.items[i]);
		}
	}

	//hack for tinywav
	if(strcmp(szPath, "AudioLib") == 0)
	{
		nob_cmd_append(&libCmd, "tinywav/tinywav.c");
	}

	//more args
	nob_cmd_append(&libCmd, "-std=gnu23", "-o");

	//make the wierdly named so file name
	char	tmp[128];
	sprintf(tmp, "../libs/lib%s.so", szPath);
	nob_cmd_append(&libCmd,
		tmp, "-lvulkan", "-lpng16", "-lSDL3",
		"../dxvk-native/build/src/dxgi/libdxvk_dxgi.so",
		"../dxvk-native/build/src/d3d11/libdxvk_d3d11.so");

	if(!nob_cmd_run(&libCmd))	return	false;

	//pop back up a dir
	if(!nob_set_current_dir(".."))
	{
		printf("Something went wrong changing to ..\n");
		return	false;
	}

	nob_da_free(cFiles);
	nob_cmd_free(libCmd);
	
	return	true;
}

int	main(int argc, char **argv)
{
	NOB_GO_REBUILD_URSELF(argc, argv);

	//ensure libs dir there
	if(!nob_mkdir_if_not_exists("libs"))
	{
		printf("Something went wrong making the libs dir...\n");
		return	EXIT_FAILURE;
	}

	if(!sBuildCLib("UtilityLib"))
	{
		return	EXIT_FAILURE;
	}
	if(!sBuildCLib("UILib"))
	{
		return	EXIT_FAILURE;
	}
	if(!sBuildCLib("TerrainLib"))
	{
		return	EXIT_FAILURE;
	}
	if(!sBuildPhysicsLib("PhysicsLib"))
	{
		return	EXIT_FAILURE;
	}
	if(!sBuildCLib("MeshLib"))
	{
		return	EXIT_FAILURE;
	}
	if(!sBuildCLib("MaterialLib"))
	{
		return	EXIT_FAILURE;
	}
	if(!sBuildCLib("InputLib"))
	{
		return	EXIT_FAILURE;
	}
	if(!sBuildCLib("AudioLib"))
	{
		return	EXIT_FAILURE;
	}
	
	return	EXIT_SUCCESS;
}