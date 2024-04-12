#pragma	once
#include	<stdint.h>


bool	SoundEffectCreate(const char *szName, const char *szPath, struct FAudio *pFA);
bool	SoundEffectPlay(const char *szName);
void	SoundEffectDestroyAll(void);
int		SoundEffectLoadAllInPath(const char *szDir, struct FAudio *pFA);