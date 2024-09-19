#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmimage/diargimg.h>

#include <string>

#include "data/VolData.h"
#include "utils/logger.h"
using namespace Voluma;

int main(int argc, const char** argv) {
    Logger::init(Logger::LoggerConfig());

    auto dcmData = VolData::loadFromDisk(argv[1]);
    logInfo("patient info: {}", dcmData->getPatientData());

    return 0;
}