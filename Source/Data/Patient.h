#include <string>

#include "Core/Enum.h"
namespace Voluma {
enum class Gender { Male, Female, Others };
struct PatientData {
    std::string id;
    std::string name;
    std::string birthDate;

    Gender gender;  // 0-male, 1-female, 2-others

    std::string toString() const;
};
VL_ENUM_INFO(Gender, {{Gender::Male, "Male"},
                      {Gender::Female, "Female"},
                      {Gender::Others, "Others"}})
VL_ENUM_REGISTER(Gender);
}  // namespace Voluma

VL_FMT(Voluma::PatientData)