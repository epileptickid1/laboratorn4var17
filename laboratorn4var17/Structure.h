#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <vector>
#include <string>
#include <mutex>

class Structure {
private:
    std::vector<int> fields;
    std::vector<std::mutex> mutexes;

public:
    explicit Structure(int num_fields);

    void set(int index, int value);

    int get(int index);

    std::string toString();
};

#endif 