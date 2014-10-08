//
//  LZMA Helpers
//  Version: 1.0
//  Date: 2014.09.17
//

#ifndef __LZMA_H__
#define __LZMA_H__

#include <LzmaLib/LzmaLib.h>

namespace lzma {

template <typename TBuffer>
struct PackedData
{
    unsigned char props[LZMA_PROPS_SIZE];
    size_t realSize;
    TBuffer data;

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

template<typename TInStream, typename TContainer>
static bool readPackedData(TInStream &s, lzma::PackedData<TContainer> &pd)
{
    // props, realSize, data.size, data

    s.read(reinterpret_cast<char *>(&pd.props[0]), LZMA_PROPS_SIZE);
    if (!s)
        return false;

    s.read(reinterpret_cast<char *>(&pd.realSize), sizeof(pd.realSize));
    if (!s)
        return false;

    size_t size;
    s.read(reinterpret_cast<char *>(&size), sizeof(size));
    if (!s)
        return false;

    pd.data.resize(size);
    s.read(reinterpret_cast<char *>(&pd.data[0]), size);
    if (!s)
        return false;

    return true;
}

template<typename TOutStream, typename TContainer>
static bool writePackedData(TOutStream &s, lzma::PackedData<TContainer> &pd)
{
    // props, realSize, data.size, data

    s.write(reinterpret_cast<const char *>(&pd.props[0]), LZMA_PROPS_SIZE);
    if (!s)
        return false;

    s.write(reinterpret_cast<const char *>(&pd.realSize), sizeof(pd.realSize));
    if (!s)
        return false;

    size_t size = pd.data.size();
    s.write(reinterpret_cast<const char *>(&size), sizeof(size));
    if (!s)
        return false;

    s.write(reinterpret_cast<const char *>(&pd.data[0]), size);
    if (!s)
        return false;

    return true;
}

}  // namespace lzma

#endif  // __LZMA_H__
