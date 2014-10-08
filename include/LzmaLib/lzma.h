//
//  LZMA Helpers
//  Version: 1.0
//  Date: 2014.09.17
//

#ifndef __LZMA_H__
#define __LZMA_H__

#include <LzmaLib/LzmaLib.h>

struct LzmaPackedData
{
    unsigned char props[LZMA_PROPS_SIZE];
    size_t realSize;
    size_t size;
    unsigned char *data;
};

// Params:
// source, size    - Uncompressed data.
// result          - Заполнять не нужно. Функция заполняет структуру
//                   самостоятельно и только если нет ошибок!
// result.data     - Возвращает указатель на буфер с сжатыми данными.
//                   Не забудьте освободить через free().
// result.size     - Возвращает размер сжатых данных.
// result.realSize - Просто дублируется входящий параметр size.
// result.props    - Возвращает внутреннюю информацию архиватора о настройках
//                   сжатия. Нужна архиватору для разархивирвоания. Скорее всего
//                   вам не понадобиться.
// Return:
// 1 - OK and 'result' filled.
// 0 - Error. В этом случае функция не устанавливает result.data и освобождать
//     ничего не нужно.
__inline
int lzmaPack(const unsigned char *source, const size_t size,
             LzmaPackedData *result)
{
    size_t bufsize = size * sizeof(unsigned char);
    unsigned char *buf = (unsigned char *)malloc(bufsize);
    size_t propSize = LZMA_PROPS_SIZE;
    while (1)
    {
        int code = LzmaCompress(buf, &bufsize, source, size, result->props,
                                &propSize, 9, 1 << 24, 3, 0, 2, 32, 2);
        if (code == SZ_OK)
        {
            // Скорее всего размер будет другим (меньше), поэтому:
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

    free(buf);
    return 0;
}

// Params:
// source     - Compressed data. Полученные ранее от lzmaCompress.
// result,
// resultSize - Возвращается указатель на распакованные данные и размер этих
//              данных.
//              Не забудьте освободить *result через free().
// Return:
// 1 - OK, 'result' and 'resultSize' filled.
// 0 - Error. В этом случае функция не устанавливает result и освобождать
//     ничего не нужно.
__inline
int lzmaUnpack(const LzmaPackedData *source, unsigned char **result,
               size_t *resultSize)
{
    const size_t DEST_SIZE = source->realSize * sizeof(unsigned char);
    size_t bufsize = DEST_SIZE;
    unsigned char *buf = (unsigned char *)malloc(bufsize);
    size_t sourceSize = source->size;
    size_t propSize = LZMA_PROPS_SIZE;
    while (1)
    {
        int code = LzmaUncompress(buf, &bufsize, source->data, &sourceSize,
                                  source->props, LZMA_PROPS_SIZE);
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

    free(buf);
    return 0;
}

#endif  // __LZMA_H__
