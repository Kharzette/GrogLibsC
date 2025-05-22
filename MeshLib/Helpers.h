#pragma once
#include	<stdint.h>
#include	<cglm/call.h>
#include	<d3d11.h>

typedef struct	GraphicsDevice_t	GraphicsDevice;


void	MakeStructuredBuffer(GraphicsDevice *pGD,
	int structSize, int numItems, void *pVData,
	ID3D11Buffer **ppBuffer, ID3D11ShaderResourceView **ppSRV);

void	MakeIBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize);