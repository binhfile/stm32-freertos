/***************************** Include Files *********************************/
#include "ringbuffer.h"

int ringbuffer_init(ringbuffer *pRingBuff,void* space, unsigned int uiSize)
{
	int iReturn = -1;
	if(pRingBuff && uiSize > 0)
	{
		pRingBuff->pcBase = (char*)space;
		pRingBuff->pcData = pRingBuff->pcBase;
		if(pRingBuff->pcData)
		{
			pRingBuff->pcReadPos = pRingBuff->pcData;
			pRingBuff->pcWritePos = pRingBuff->pcData;
			pRingBuff->uiIsFull = 0;
			pRingBuff->uiSize = uiSize;
			iReturn = 0;
		}
	}
	return iReturn;
}

int ringbuffer_destroy(ringbuffer *pRingBuff)
{
	int iReturn = -1;
	if(pRingBuff)
	{
		pRingBuff->pcData = 0;
		pRingBuff->pcReadPos = 0;
		pRingBuff->pcWritePos = 0;
		pRingBuff->uiSize = 0;
		iReturn = 0;
	}
	return iReturn;
}

unsigned int ringbuffer_write(ringbuffer *pRingBuff, const unsigned char *pcData, unsigned int uiSize)
{
	unsigned int iReturn = 0;
	int iIndex;
	int iInvalidSize = 0;
	ringbuffer ringbuff;

	if(pRingBuff && pcData && uiSize)
	{
		ringbuff = *pRingBuff;
		iInvalidSize = ringbuffer_freesize(&ringbuff);
		if(iInvalidSize > 0)
		{
			iInvalidSize = (iInvalidSize > (int)uiSize) ? uiSize : iInvalidSize;
			for(iIndex = 0; iIndex < iInvalidSize; iIndex ++ )
			{
				*pRingBuff->pcWritePos = pcData[iIndex];
				pRingBuff->pcWritePos ++;
				if(pRingBuff->pcWritePos > pRingBuff->pcData + pRingBuff->uiSize-1)
					pRingBuff->pcWritePos = pRingBuff->pcData;
			}
			if(pRingBuff->pcWritePos == pRingBuff->pcReadPos)
				pRingBuff->uiIsFull = 1;
			iReturn = iInvalidSize;
		}
	}
	return iReturn;
}

unsigned int ringbuffer_read(ringbuffer *pRingBuff, unsigned char *pcDataOut, unsigned int uiSize)
{
	unsigned int iReturn = 0;
	unsigned int iValidSize;
	unsigned int iIndex;
	ringbuffer ringbuff;
	if(pRingBuff && uiSize > 0)
	{
		ringbuff = *pRingBuff;
		iValidSize = ringbuffer_datasize(&ringbuff);
		iValidSize = (iValidSize > uiSize) ? uiSize : iValidSize;
		for(iIndex = 0; iIndex < iValidSize; iIndex++)
		{
			if(pcDataOut)
				pcDataOut[iIndex] = *pRingBuff->pcReadPos;
			pRingBuff->pcReadPos++;
			if(pRingBuff->pcReadPos > pRingBuff->pcData + pRingBuff->uiSize - 1)
				pRingBuff->pcReadPos = pRingBuff->pcData;
		}
		if(pRingBuff->pcWritePos == pRingBuff->pcReadPos)
			pRingBuff->uiIsFull = 0;
		iReturn = iValidSize;
	}
	return iReturn;
}

unsigned int ringbuffer_freesize(const ringbuffer *pRingBuff)
{
	unsigned int iInvalidSize = 0;
	if(pRingBuff)
	{
		if(pRingBuff->pcReadPos == pRingBuff->pcWritePos)
		{
			if(pRingBuff->uiIsFull)
				iInvalidSize = 0;
			else
				iInvalidSize = pRingBuff->uiSize;
		}
		else
		{
			if(pRingBuff->pcWritePos > pRingBuff->pcReadPos)
			{
				iInvalidSize = pRingBuff->uiSize + pRingBuff->pcReadPos - pRingBuff->pcWritePos;
			}
			else
				iInvalidSize = pRingBuff->pcReadPos - pRingBuff->pcWritePos;
		}
	}
	return iInvalidSize;
}

unsigned int ringbuffer_datasize(const ringbuffer *pRingBuff)
{
	unsigned int iValidSize = 0;
	ringbuffer ringbuff;
	if(pRingBuff)
	{
		ringbuff = *pRingBuff;
		if(ringbuff.pcReadPos == ringbuff.pcWritePos)
		{
			if(ringbuff.uiIsFull)
				iValidSize = ringbuff.uiSize;
			else
				iValidSize = 0;
		}
		else
		{
			if(ringbuff.pcReadPos > ringbuff.pcWritePos)
			{
				iValidSize = ringbuff.uiSize + ringbuff.pcWritePos- ringbuff.pcReadPos;
			}
			else
				iValidSize =ringbuff.pcWritePos - ringbuff.pcReadPos;
		}
	}
	return iValidSize;
}
unsigned int ringbuffer_clear(ringbuffer *pRingBuff)
{
	int iReturn = -1;
	if(pRingBuff)
	{
		pRingBuff->pcData = pRingBuff->pcBase;
		pRingBuff->pcReadPos = pRingBuff->pcData;
		pRingBuff->pcWritePos = pRingBuff->pcData;
		pRingBuff->uiIsFull = 0;
		iReturn = 0;
	}
	return iReturn;
}
unsigned char ringbuffer_at(ringbuffer *pRingBuff, int iIndex)
{
	int iLen = ringbuffer_datasize(pRingBuff);
	char *pPointer = pRingBuff->pcReadPos;
	if(iIndex >=0 && iIndex < iLen)
	{
		while(iIndex > 0)
		{
			pPointer++;
			if(pPointer > pRingBuff->pcData + pRingBuff->uiSize - 1)
				pPointer = pRingBuff->pcData;
			iIndex--;
		}
	}
	return (unsigned char)(*(pPointer));
}
