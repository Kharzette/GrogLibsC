#pragma once

typedef struct	Audio_t Audio;
typedef float	vec3[3];

Audio	*Audio_Create(int deviceIndex);
Audio	*Audio_CreateInteractive(void);

bool	Audio_SFXCreate(Audio *pAud,
			const char *szName, const char *szPath,
            uint32_t numChannels, int *pIdx);

void    Audio_Destroy(Audio **ppAud);

void	Audio_Update(Audio *pAud, vec3 position, vec3 velocity);
