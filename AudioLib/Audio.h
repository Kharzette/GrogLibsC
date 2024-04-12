#pragma once

typedef struct	Audio_t Audio;


Audio	*Audio_Create(int deviceIndex);
Audio	*Audio_CreateInteractive(void);

void    Audio_Destroy(Audio **ppAud);