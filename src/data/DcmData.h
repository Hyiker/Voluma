#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcmetinf.h>
#include <dcmtk/dcmdata/dctk.h>

namespace Voluma {
class DcmData {
   public:
    DcmData() = default;
    static DcmData loadFromDisk(const std::string& filename);

   private:
    DcmData(DcmData* pData, DcmMetaInfo* mpMeta);

    DcmData* mpData;
    DcmMetaInfo* mpMeta;
};
}  // namespace Voluma