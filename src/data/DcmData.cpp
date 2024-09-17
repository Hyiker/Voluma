#include "DcmData.h"

#include "utils/logger.h"
namespace Voluma {
DcmData DcmData::loadFromDisk(const std::string& filename) {
    DcmFileFormat dfile;

    OFCondition result = dfile.loadFile(filename.c_str());
    if (result.bad()) logError("Failed to load file.");

}
}  // namespace Voluma