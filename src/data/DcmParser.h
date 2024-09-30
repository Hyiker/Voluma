#pragma once

#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>

namespace Voluma {
class DcmParser {
   public:
    DcmParser(DcmDataset* pDataset) : mpDataset(pDataset) {}

    std::string getString(const DcmTagKey& key) const;
    uint16_t getU16(const DcmTagKey& key) const;
    int getInt(const DcmTagKey& key) const;
    double getF64(const DcmTagKey& key) const;

    std::vector<uint16_t> getU16Array(const DcmTagKey& key) const;

   private:
    DcmDataset* mpDataset;
};
}  // namespace Voluma