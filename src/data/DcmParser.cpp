#include "DcmParser.h"

#include <dcmtk/ofstd/oftypes.h>

#include <cstdint>
#include <vector>
namespace Voluma {
std::string DcmParser::getString(const DcmTagKey& key) const {
    OFString value;
    if (mpDataset->findAndGetOFString(key, value).good()) {
        return value.c_str();
    }
    return "";
}

uint16_t DcmParser::getU16(const DcmTagKey& key) const {
    Uint16 val;
    mpDataset->findAndGetUint16(key, val);
    return uint16_t(val);
}

int DcmParser::getInt(const DcmTagKey& key) const {
    Sint32 val;
    mpDataset->findAndGetSint32(key, val);
    return int(val);
}

double DcmParser::getF64(const DcmTagKey& key) const {
    Float64 val;
    mpDataset->findAndGetFloat64(key, val);
    return double(val);
}

std::vector<uint16_t> DcmParser::getU16Array(const DcmTagKey& key) const {
    const Uint16* pData = nullptr;
    unsigned long cnt;

    mpDataset->findAndGetUint16Array(key, pData, &cnt);
    if (!pData) {
        return {};
    }

    return std::vector<uint16_t>(pData, pData + cnt);
}

}  // namespace Voluma