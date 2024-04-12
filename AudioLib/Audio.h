#pragma once

typedef struct	Audio_t Audio;


Audio	*Audio_Create(int deviceIndex);
Audio	*Audio_CreateInteractive(void);

void    Audio_Destroy(Audio **ppAud);

void	Audio_Update(Audio *pAud, vec3 position, vec3 velocity);