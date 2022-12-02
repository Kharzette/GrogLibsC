#include	<stdint.h>

struct	FAudio;


bool	SoundEffectCreate(const char *szName, const char *szPath, FAudio *pFA);
bool	SoundEffectPlay(const char *szName);
void	SoundEffectDestroyAll(void);