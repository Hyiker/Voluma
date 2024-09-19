#include "VolData.h"

#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcmetinf.h>
#include <dcmtk/ofstd/ofstring.h>
#include <dcmtk/ofstd/oftypes.h>

#include <memory>
#include <optional>
#include <string>

#include "core/error.h"
#include "utils/logger.h"
namespace Voluma {

// 将图像数据保存为PPM文件
void saveImageAsPPM(const std::string& filename, const Uint16* pixelData,
                    int width, int height) {
    std::ofstream file(filename, std::ios::binary);

    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    // PPM header
    file << "P6\n" << width << " " << height << "\n65535\n";

    // 写入像素数据（16位）
    for (int i = 0; i < width * height; ++i) {
        Uint16 pixel = pixelData[i];
        Uint16 r = pixel;  // R通道
        Uint16 g = pixel;  // G通道
        Uint16 b = pixel;  // B通道

        file.write(reinterpret_cast<char*>(&r), sizeof(Uint16));
        file.write(reinterpret_cast<char*>(&g), sizeof(Uint16));
        file.write(reinterpret_cast<char*>(&b), sizeof(Uint16));
    }

    file.close();
    std::cout << "Image saved as: " << filename << std::endl;
}
std::shared_ptr<VolData> VolData::loadFromDisk(const std::string& filename) {
    DcmFileFormat dfile;

    OFCondition result = dfile.loadFile(filename.c_str());
    if (result.bad()) logError("Failed to load file.");

    return std::make_shared<VolData>(dfile);
}

VolData::VolData(DcmFileFormat& dcmFile) {
    auto* pMeta = dcmFile.getMetaInfo();
    auto* pDataset = dcmFile.getDataset();

    VL_ASSERT(pDataset != nullptr);
    VL_ASSERT(pMeta != nullptr);

    // Read and load metadata
    loadPatientData(pDataset);
    loadScanMeta(pDataset);

    // Read slice images

    const Uint16* pixelData = nullptr;
    Uint16 rows = 0, cols = 0;
    int frameNumber = 0;  // 选择第几层

    pDataset->findAndGetUint16(DCM_Rows, rows);
    pDataset->findAndGetUint16(DCM_Columns, cols);

    unsigned long cnt;
    pDataset->findAndGetUint16Array(DCM_PixelData, pixelData, &cnt);

    logInfo("Count: {}, {}x{}", cnt, rows, cols);
    if (pixelData == nullptr) {
        std::cerr << "Failed to extract pixel data from DICOM file."
                  << std::endl;
    }

    OFString pixelSpacing;
    if (pDataset->findAndGetOFStringArray(DCM_PixelSpacing, pixelSpacing)
            .good()) {
        logInfo("Pixel spacing: {}", pixelSpacing.c_str());
    }

    Float64 sliceThickness;
    if (pDataset->findAndGetFloat64(DCM_SliceThickness, sliceThickness)
            .good()) {
        logInfo("Slice thickness: {}", sliceThickness);
    }

    Float64 sliceLocation;
    if (pDataset->findAndGetFloat64(DCM_SliceLocation, sliceLocation).good()) {
        logInfo("Slice location: {}", sliceLocation);
    }

    const Uint16* selectedLayer = pixelData + frameNumber * rows * cols;

    saveImageAsPPM("output.ppm", selectedLayer, cols, rows);
}
static std::optional<std::string> getDatasetString(DcmDataset* pDataset,
                                                   const DcmTagKey& key) {
    OFString value;
    if (pDataset->findAndGetOFString(key, value).good()) {
        return value.c_str();
    }
    return "";
}

void VolData::loadPatientData(DcmDataset* pDataset) {
    auto getDatasetString =
        [pDataset](const DcmTagKey& key) -> std::optional<std::string> {
        OFString value;
        if (pDataset->findAndGetOFString(key, value).good()) {
            return value.c_str();
        }
        return std::nullopt;
    };

    // Initialize patient data
    std::string id, name, birthDate;

    id = getDatasetString(DCM_PatientID).value_or("");
    name = getDatasetString(DCM_PatientName).value_or("");
    birthDate = getDatasetString(DCM_PatientBirthDate).value_or("");

    std::string genderStr = getDatasetString(DCM_PatientSex).value_or("");
    auto gender = (genderStr == "M")   ? Gender::Male
                  : (genderStr == "F") ? Gender::Female
                                       : Gender::Others;

    mPatientData.id = id;
    mPatientData.name = name;
    mPatientData.birthDate = birthDate;
    mPatientData.gender = gender;
}

void VolData::loadScanMeta(DcmDataset* pDataset) {}

}  // namespace Voluma