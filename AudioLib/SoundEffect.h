#pragma	once
#include	<stdint.h>


bool	SoundEffectCreate(const char *szName, const char *szPath,
            struct FAudio *pFA, uint32_t numChannels);
bool	SoundEffectPlayIdx(int idx, vec3 position);            
bool	SoundEffectPlay(const char *szName, vec3 position);
void	SoundEffectDestroyAll(void);
int		SoundEffectLoadAllInPath(const char *szDir, struct FAudio *pFA, uint32_t numChannels);
//void	SoundEffectUpdateEmitters(struct F3DAUDIO_HANDLE h3d, const struct F3DAUDIO_LISTENER *pList);