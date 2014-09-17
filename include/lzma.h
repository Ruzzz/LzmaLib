//
//  LZMA Helpers
//  Version: 1.0, 2014.09.17
//  Author: Ruzzz [ruzzzua@gmail.com]
//

#ifndef __LZMA_H
#define __LZMA_H

#include "LzmaLib.h"

// #ifndef __cplusplus // TODO

struct LzmaPackedData
{
	unsigned char *data;
	size_t size;
	size_t realSize;
	unsigned char props[LZMA_PROPS_SIZE];	
};

// Params:
//   source, size - Uncompressed data
//   result - ��������� �� �����. ������� ��������� ��������� �������������� � ������ ���� ��� ������!
//     result.compressedData - ���������� ��������� �� ����� � ������� �������. �� �������� ���������� ����� free().
//     result.compressedSize - ���������� ������ ������ ������.
//     result.uncompressedSize - ������ ����������� �������� �������� size.
//     result.prop - ���������� ���������� ���������� ���������� � ���������� ������.
//                   ����� ���������� ��� ����������������. ������ ����� ��� �� ������������.
//     
// Return:
//   1 - OK and 'result' filled.
//   0 - Error. � ���� ������ ������� �� ������������� result.compressedData � ����������� ������ �� �����.
__inline
int lzmaPack(const unsigned char *source, const size_t size, LzmaPackedData *result)
{
	size_t bufsize = size * sizeof(unsigned char);
	unsigned char *buf = (unsigned char *)malloc(bufsize);
	size_t propSize = LZMA_PROPS_SIZE;
	while (1)
	{
		int code = LzmaCompress(buf, &bufsize, source, size, result->props, &propSize, 9, 1 << 24, 3, 0, 2, 32, 2);  // TODO Magic consts
		if (code == SZ_OK)
		{
			// ������ ����� ������ ����� ������ (������), �������:
			unsigned char *newbuf = (unsigned char *)realloc(buf, bufsize);
			if (!newbuf)
				break;  // Error
			result->data = newbuf;
			result->size = bufsize;
			result->realSize = size;
			return 1;
		}
		else if (code == SZ_ERROR_OUTPUT_EOF)
		{
			bufsize = bufsize + (bufsize >> 1);  // * 1.5
			unsigned char *newbuf = (unsigned char *)realloc(buf, bufsize);
			if (!newbuf)
				break;  // Error
			buf = newbuf;  // And try compress
		}
		else
			break;  // Error
	}
	free(buf);  // If error
	return 0;
}

// Params:
//   source - Compressed data. ���������� ����� �� lzmaCompress.
//   result, resultSize - ������������ ��������� �� ������������� ������ � ������ ���� ������. �� �������� ���������� ����� free().
//     
// Return:
//   1 - OK, 'result' and 'resultSize' filled.
//   0 - Error. � ���� ������ ������� �� ������������� result � ����������� ������ �� �����.
__inline
int lzmaUnpack(const LzmaPackedData *source, unsigned char **result, size_t *resultSize)
{
	const size_t DEST_SIZE = source->realSize * sizeof(unsigned char);
	size_t bufsize = DEST_SIZE;
	unsigned char *buf = (unsigned char *)malloc(bufsize);
	size_t sourceSize = source->size;
	size_t propSize = LZMA_PROPS_SIZE;
	while (1)
	{
		int code = LzmaUncompress(buf, &bufsize, source->data, &sourceSize, source->props, LZMA_PROPS_SIZE);
		if (code == SZ_OK)
		{
			if (bufsize != DEST_SIZE)
			{
				unsigned char *newbuf = (unsigned char *)realloc(buf, bufsize);
				if (!newbuf)
					break;  // Error
				buf = newbuf;
			}

			*result = buf;
			*resultSize = bufsize;
			return 1;
		}
		else if (code == SZ_ERROR_OUTPUT_EOF)
		{
			bufsize = bufsize + 1024;  // + 1Kb
			unsigned char *newbuf = (unsigned char *)realloc(buf, bufsize);
			if (!newbuf)
				break;  // Error
			buf = newbuf;  // And try Uncompress
		}
		else
			break;  // Error
	}
	free(buf);  // If error
	return 0;
}

// #else  // #ifndef __cplusplus // TODO

namespace lzma
{
	// TODO ��������� ������������� ����� lzmaCompress � lzmaUncompress, ������������ #ifndef __cplusplus
	
	template <typename TBuffer>
	struct PackedData
	{
		TBuffer data;
		size_t realSize;
		unsigned char props[LZMA_PROPS_SIZE];

		bool pack(const TBuffer source)
		{
			LzmaPackedData pd;
			if (!lzmaPack((unsigned char*)&source[0], source.size(), &pd))
				return false;
			data.assign((TBuffer::value_type *)pd.data, (TBuffer::value_type *)(pd.data + pd.size));
			free(pd.data);
			realSize = pd.realSize;
			memcpy(props, pd.props, LZMA_PROPS_SIZE);
			return true;
		}

		bool unpack(TBuffer &result)
		{
			LzmaPackedData pd = {
				(unsigned char*)&data[0],
				data.size(),
				realSize
			};
			memcpy(pd.props, props, LZMA_PROPS_SIZE);
			unsigned char *buf;
			size_t bufsize;
			if (!lzmaUnpack(&pd, &buf, &bufsize))
				return false;
			result.assign((TBuffer::value_type *)buf, (TBuffer::value_type *)(buf + bufsize));
			free(buf);
			return true;
		}
	};

}  // namespace lzma

// #endif  // __cplusplus // TODO

#endif  // __LZMA_H
