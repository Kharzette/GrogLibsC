#pragma	once
#include	<stdint.h>


bool	SoundEffect_Create(const char *szName, const char *szPath,
            struct FAudio *pFA, uint32_t numChannels, int *pIdx);
void	SoundEffect_DestroyAll(void);
int		SoundEffect_LoadAllInPath(const char *szDir, struct FAudio *pFA, uint32_t numChannels);

const char  *SoundEffect_GetIndexName(int index);
int         SoundEffect_GetSFXCount(void);

bool	SoundEffect_PlayIdx(int idx, vec3 position);            
bool	SoundEffect_Play(const char *szName, vec3 position);
//void	SoundEffect_UpdateEmitters(struct F3DAUDIO_HANDLE h3d, const struct F3DAUDIO_LISTENER *pList);