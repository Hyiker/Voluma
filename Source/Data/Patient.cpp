#include "Patient.h"

namespace Voluma {
std::string PatientData::toString() const {
    return fmt::format("PatientData(id={}, name={}, birthDate={}, gender={})",
                       id, name, birthDate, gender);
}
}  // namespace Voluma