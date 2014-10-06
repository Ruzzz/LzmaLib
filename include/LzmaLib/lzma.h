//
//  LZMA Helpers
//  Version: 1.0
//  Date: 2014.09.17
//

#ifndef __LZMA_H__
#define __LZMA_H__

#include <LzmaLib/LzmaLib.h>

#ifndef __cplusplus

struct LzmaPackedData
{
    unsigned char *data;
    size_t size;
    size_t realSize;
    unsigned char props[LZMA_PROPS_SIZE];    
};

// Params:
//   source, size - Uncompressed data
//   result - Заполнять не нужно. Функция заполняет структуру самостоятельно и только если нет ошибок!
//     result.compressedData - Возвращает указатель на буфер с сжатыми данными. Не забудьте освободить через free().
//     result.compressedSize - Возвращает размер сжатых данных.
//     result.uncompressedSize - Просто дублируется входящий параметр size.
//     result.prop - Возвращает внутреннюю информацию архиватора о настройках сжатия.
//                   Нужна архиватору для разархивирвоания. Скорее всего вам не понадобиться.
// Return:
//   1 - OK and 'result' filled.
//   0 - Error. В этом случае функция не устанавливает result.compressedData и освобождать ничего не нужно.
__inline
int lzmaPack(const unsigned char *source, const size_t size, LzmaPackedData *result)
{
    size_t bufsize = size * sizeof(unsigned char);
    unsigned char *buf = (unsigned char *)malloc(bufsize);
    size_t propSize = LZMA_PROPS_SIZE;
    while (1)
    {
        int code = LzmaCompress(buf, &bufsize, source, size, result->props, &propSize, 9, 1 << 24, 3, 0, 2, 32, 2);
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
//   source - Compressed data. Полученные ранее от lzmaCompress.
//   result, resultSize - Возвращается указатель на распакованные данные и размер этих данных. Не забудьте освободить через free().
// Return:
//   1 - OK, 'result' and 'resultSize' filled.
//   0 - Error. В этом случае функция не устанавливает result и освобождать ничего не нужно.
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

    free(buf);
    return 0;
}

#else  // #ifndef __cplusplus

namespace lzma
{
    template <typename TBuffer>
    struct PackedData
    {
        TBuffer data;
        size_t realSize;
        unsigned char props[LZMA_PROPS_SIZE];

        bool pack(const TBuffer source)
        {
            size_t packedSize = source.size();
            data.resize(packedSize);
            size_t propSize = LZMA_PROPS_SIZE;
            while (1)
            {
                int code = LzmaCompress((unsigned char *)&data[0], &packedSize,
                                        (unsigned char *)&source[0], source.size(),
                                        props, &propSize, 9, 1 << 24, 3, 0, 2, 32, 2);
                if (code == SZ_OK)
                {
                    data.resize(packedSize);
                    // data.shrink_to_fit();
                    realSize = source.size();
                    return true;
                }
                else if (code == SZ_ERROR_OUTPUT_EOF)
                {
                    packedSize = packedSize + (packedSize >> 1);  // * 1.5
                    data.resize(packedSize);
                }
                else
                    break;  // Error
            }

            data.clear();
            // data.shrink_to_fit();
            return false;
        }

        bool unpack(TBuffer &result)
        {
            size_t resultSize = realSize;
            result.resize(resultSize);
            size_t packedSize = data.size();
            size_t propSize = LZMA_PROPS_SIZE;
            while (1)
            {
                int code = LzmaUncompress((unsigned char*)&result[0], &resultSize,
                                          (unsigned char*)&data[0], &packedSize,
                                          props, LZMA_PROPS_SIZE);
                if (code == SZ_OK)
                {
                    if (resultSize != realSize)
                    {
                        result.resize(resultSize);
                        // data.shrink_to_fit();
                    }
                    return true;
                }
                else if (code == SZ_ERROR_OUTPUT_EOF)
                {
                    resultSize = resultSize + 1024;  // + 1Kb
                    result.resize(resultSize);
                }
                else
                    break;  // Error
            }

            result.clear();
            // result.shrink_to_fit();
            return false;
        }
    };

    template<typename TInStream, typename TByteContainer>
    static bool readPackedData(TInStream &stream, lzma::PackedData<TByteContainer> &pd)
    {
        // props, realSize, data.size, data

        stream.read(reinterpret_cast<char *>(&pd.props[0]), LZMA_PROPS_SIZE);
        if (!stream)
            return false;

        stream.read(reinterpret_cast<char *>(&pd.realSize), sizeof(pd.realSize));
        if (!stream)
            return false;

        size_t size;
        stream.read(reinterpret_cast<char *>(&size), sizeof(size));
        if (!stream)
            return false;

        pd.data.resize(size);
        stream.read(reinterpret_cast<char *>(&pd.data[0]), size);
        if (!stream)
            return false;

        return true;
    }

    template<typename TOutStream, typename TByteContainer>
    static bool writePackedData(TOutStream &stream, lzma::PackedData<TByteContainer> &pd)
    {
        // props, realSize, data.size, data

        stream.write(reinterpret_cast<const char *>(&pd.props[0]), LZMA_PROPS_SIZE);
        if (!stream)
            return false;

        stream.write(reinterpret_cast<const char *>(&pd.realSize), sizeof(pd.realSize));
        if (!stream)
            return false;

        size_t size = pd.data.size();
        stream.write(reinterpret_cast<const char *>(&size), sizeof(size));
        if (!stream)
            return false;

        stream.write(reinterpret_cast<const char *>(&pd.data[0]), size);
        if (!stream)
            return false;

        return true;
    }

}  // namespace lzma

#endif  // __cplusplus // TODO

#endif  // __LZMA_H__
