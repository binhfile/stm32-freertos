#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

/***************************** Include Files *********************************/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct ringbuffer_st
{
	char *pcBase;
	char *pcWritePos;
	char *pcReadPos;
	char *pcData;
	unsigned int uiSize;
	unsigned int uiIsFull;
}ringbuffer;
/************************** Function Prototypes ******************************/
/** @brief Initialize Ring buffer
 *
 *  @param	pRingBuff	Pointer to ringbuffer variable
 *  @param	uiSize		Fixed-size of ring buffer
 *  @return	int			negative if error, 0 if success
 *  @note
 */
int ringbuffer_init(ringbuffer *pRingBuff, void* space ,unsigned int uiSize);
/** @brief Destroy ring buffer
 *
 *  @param	pRingBuff	Pointer to ringbuffer variable
 *  @return	int			negative if error, 0 if success
 *  @note
 */
int ringbuffer_destroy(ringbuffer *pRingBuff);
/** @brief Write data to ring buffer
 *
 *  @param	pRingBuff	Pointer to ringbuffer variable
 *  @param  pcData		Pointer to data
 *  @param	uiSize		Size in byte of data
 *  @return	int			number of byte is written
 *  @note
 */
unsigned int ringbuffer_write(ringbuffer *pRingBuff, const unsigned char *pcData, unsigned int uiSize);
/** @brief Read data from ring buffer
 *
 *  @param	pRingBuff	Pointer to ringbuffer variable
 *  @param  pcDataOut	Pointer to storage space
 *  @param	uiSize		Number of byte must read
 *  @return	int			number of byte is read
 *  @note
 */
unsigned int ringbuffer_read(ringbuffer *pRingBuff, unsigned char *pcDataOut, unsigned int uiSize);
/** @brief Get free size data of ring buffer
 *
 *  @param	pRingBuff	Pointer to ringbuffer variable
 *  @return	int			number of byte is free
 *  @note
 */
unsigned int ringbuffer_freesize(const ringbuffer *pRingBuff);
/** @brief Get size data of ring buffer
 *
 *  @param	pRingBuff	Pointer to ringbuffer variable
 *  @return	int			Number of byte data's
 *  @note
 */
unsigned int ringbuffer_datasize(const ringbuffer *pRingBuff);
unsigned int ringbuffer_clear(ringbuffer *pRingBuff);
unsigned char ringbuffer_at(ringbuffer *pRingBuff, int iIndex);

#endif /* ringbuffer_H_ */
