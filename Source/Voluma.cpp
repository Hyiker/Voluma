#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmimage/diargimg.h>

#include "Core/SampleApp.h"
#include "Data/VolData.h"
#include "Utils/Logger.h"

using namespace Voluma;

int main(int argc, const char **argv) {
  Logger::init(Logger::LoggerConfig());

  SampleApp app;
  //   auto dcmData = VolData::loadFromDisk(argv[1]);
  //   logInfo("Slice count: {}", dcmData->getSliceCount());
  //   logInfo("patient info: {}", dcmData->getPatientData());
  //   logInfo("scan meta: {}", dcmData->getScanMetaData());
  //   dcmData->saveSlice("output.exr", 0);

  return 0;
}