#include "Structure.h"
#include <sstream> 
#include <stdexcept> 

Structure::Structure(int num_fields)
    : fields(num_fields, 0),
    mutexes(num_fields)
{
}

void Structure::set(int index, int value) {
    if (index < 0 || index >= fields.size()) return;

    std::lock_guard<std::mutex> lock(mutexes[index]);
    fields[index] = value;
}

int Structure::get(int index) {
    if (index < 0 || index >= fields.size()) return -1;

    std::lock_guard<std::mutex> lock(mutexes[index]);
    return fields[index];
}

std::string Structure::toString() {
    if (mutexes.size() != 3) return "Error: Expected 3 mutexes";

    std::scoped_lock lock(mutexes[0], mutexes[1], mutexes[2]);

    std::stringstream ss;
    ss << "Fields: [";
    for (size_t i = 0; i < fields.size(); ++i) {
        ss << fields[i];
        if (i < fields.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}