#ifndef __FILE_FORMAT_H__
#define __FILE_FORMAT_H__
#include <stdint.h>

#define AUDIO_MAGIC_NUMBER 0xA77CA5FE
struct audio_data_header
{
	uint32_t Magic;
	uint32_t EntryCount;
};

struct audio_data_entry
{
	uint32_t Offset;
	uint32_t Size;
	
	uint8_t Name[8];
};

#endif