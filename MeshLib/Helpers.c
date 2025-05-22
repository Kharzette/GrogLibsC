#include	<d3d11.h>
#include	"../UtilityLib/GraphicsDevice.h"


void	MakeStructuredBuffer(GraphicsDevice *pGD,
	int structSize, int numItems, void *pVData,
	ID3D11Buffer **ppBuffer, ID3D11ShaderResourceView **ppSRV)
{
	//buffer is not optional here
	assert(ppBuffer != NULL);

	D3D11_BUFFER_DESC	bufDesc;
	memset(&bufDesc, 0, sizeof(D3D11_BUFFER_DESC));

	//particle buffer
	bufDesc.BindFlags			=D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufDesc.ByteWidth			=structSize * numItems;
	bufDesc.CPUAccessFlags		=0;
	bufDesc.MiscFlags			=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bufDesc.StructureByteStride	=structSize;
	bufDesc.Usage				=D3D11_USAGE_DEFAULT;

	*ppBuffer	=GD_CreateBufferWithData(pGD, &bufDesc,
					pVData, structSize * numItems);

	if(ppSRV != NULL)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC	srvDesc;
		memset(&srvDesc, 0, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));

		srvDesc.Format				=DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension		=D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementWidth	=numItems;

		*ppSRV	=GD_CreateSRV(pGD, (ID3D11Resource *)*ppBuffer, &srvDesc);
	}
}

void	MakeIBDesc(D3D11_BUFFER_DESC *pDesc, uint32_t byteSize)
{
	memset(pDesc, 0, sizeof(D3D11_BUFFER_DESC));

	pDesc->BindFlags			=D3D11_BIND_INDEX_BUFFER;
	pDesc->ByteWidth			=byteSize;
	pDesc->CPUAccessFlags		=DXGI_CPU_ACCESS_NONE;
	pDesc->MiscFlags			=0;
	pDesc->StructureByteStride	=0;
	pDesc->Usage				=D3D11_USAGE_IMMUTABLE;
}