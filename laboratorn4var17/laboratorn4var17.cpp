#include <iostream>
#include <vector>
#include <string>
#include <mutex>          
#include <thread>         
#include <sstream>        
#include <fstream>        
#include <chrono>         
#include <random>         
#include <map>            
#include <iomanip>        

#include "Structure.h"

enum class OpType { READ, WRITE, STRING };
struct Command {
    OpType type;
    int field_index = 0;
    int value = 0;
};

std::vector<Command> load_commands(const std::string& filename) {
    std::vector<Command> commands;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return commands;
    }

    std::string line;
    std::string op_name;
    int field_index;
    int value;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        ss >> op_name;
        if (op_name == "read") {
            ss >> field_index;
            commands.push_back({ OpType::READ, field_index, 0 });
        }
        else if (op_name == "write") {
            ss >> field_index >> value;
            commands.push_back({ OpType::WRITE, field_index, value });
        }
        else if (op_name == "string") {
            commands.push_back({ OpType::STRING, 0, 0 });
        }
    }
    return commands;
}

void generate_file(const std::string& filename,
    long num_ops,
    const std::map<std::string, double>& weights) {

    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not create file " << filename << std::endl;
        return;
    }

    std::vector<std::string> operations;
    std::vector<double> probabilities;

    for (const auto& pair : weights) {
        operations.push_back(pair.first);
        probabilities.push_back(pair.second);
    }

    std::mt19937 gen(std::random_device{}());
    std::discrete_distribution<> dist(probabilities.begin(), probabilities.end());

    for (long i = 0; i < num_ops; ++i) {
        int index = dist(gen);
        std::string op = operations[index];

        if (op.find("write") != std::string::npos) {
            file << op << " 1\n";
        }
        else {
            file << op << "\n";
        }
    }
}

void worker(Structure& structure, const std::vector<Command>& commands) {
    for (const auto& cmd : commands) {
        switch (cmd.type) {
        case OpType::READ:
            structure.get(cmd.field_index);
            break;
        case OpType::WRITE:
            structure.set(cmd.field_index, cmd.value);
            break;
        case OpType::STRING:
            structure.toString();
            break;
        }
    }
}

double run_test(int num_threads, const std::vector<std::vector<Command>>& thread_commands) {
    Structure structure(3);
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker, std::ref(structure), std::cref(thread_commands[i]));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration_ms = end - start;
    return duration_ms.count();
}


int main() {
    const long NUM_OPS_PER_FILE = 200000;
    const int NUM_RUNS = 5;

    std::cout << "Generating test files..." << std::endl;

    std::map<std::string, double> weights_a = {
        {"read 0", 0.10}, {"write 0", 0.10},
        {"read 1", 0.10}, {"write 1", 0.10},
        {"read 2", 0.40}, {"write 2", 0.05},
        {"string", 0.15}
    };

    double equal_prob = 1.0 / 7.0;
    std::map<std::string, double> weights_b = {
        {"read 0", equal_prob}, {"write 0", equal_prob},
        {"read 1", equal_prob}, {"write 1", equal_prob},
        {"read 2", equal_prob}, {"write 2", equal_prob},
        {"string", equal_prob}
    };

    std::map<std::string, double> weights_c = {
        {"read 0", 0.01}, {"write 0", 0.01},
        {"read 1", 0.01}, {"write 1", 0.01},
        {"read 2", 0.01}, {"write 2", 0.05},
        {"string", 0.90}
    };

    generate_file("a_1.txt", NUM_OPS_PER_FILE, weights_a);
    generate_file("a_2.txt", NUM_OPS_PER_FILE, weights_a);
    generate_file("a_3.txt", NUM_OPS_PER_FILE, weights_a);

    generate_file("b_1.txt", NUM_OPS_PER_FILE, weights_b);
    generate_file("b_2.txt", NUM_OPS_PER_FILE, weights_b);
    generate_file("b_3.txt", NUM_OPS_PER_FILE, weights_b);

    generate_file("c_1.txt", NUM_OPS_PER_FILE, weights_c);
    generate_file("c_2.txt", NUM_OPS_PER_FILE, weights_c);
    generate_file("c_3.txt", NUM_OPS_PER_FILE, weights_c);

    std::cout << "Loading commands from files..." << std::endl;

    std::map<std::string, std::vector<std::vector<Command>>> scenarios;
    scenarios["A (Variant 9)"] = {
        load_commands("a_1.txt"),
        load_commands("a_2.txt"),
        load_commands("a_3.txt")
    };
    scenarios["B (Equal Freq)"] = {
        load_commands("b_1.txt"),
        load_commands("b_2.txt"),
        load_commands("b_3.txt")
    };
    scenarios["C ('string' Spam)"] = {
        load_commands("c_1.txt"),
        load_commands("c_2.txt"),
        load_commands("c_3.txt")
    };

    std::cout << "Running performance tests (" << NUM_RUNS << " runs for averaging)..." << std::endl;
    std::cout << std::fixed << std::setprecision(2);

    std::map<std::string, std::map<int, double>> results;

    for (int run = 1; run <= NUM_RUNS; ++run) {
        std::cout << "--- Run " << run << " ---" << std::endl;
        for (const auto& [name, thread_commands] : scenarios) {
            for (int num_threads : {1, 2, 3}) {
                double time_ms = run_test(num_threads, thread_commands);
                std::cout << "  " << std::setw(18) << std::left << name
                    << " | Threads: " << num_threads
                    << " | Time: " << std::setw(8) << std::right << time_ms << " ms" << std::endl;
                results[name][num_threads] += time_ms;
            }
        }
    }
    std::cout << "----------------------------------------------------" << std::endl;
    std::cout << std::setw(25) << std::left << "Scenario / Threads"
        << std::setw(10) << std::right << "1 Thread"
        << std::setw(10) << std::right << "2 Threads"
        << std::setw(10) << std::right << "3 Threads" << std::endl;
    std::cout << "----------------------------------------------------" << std::endl;

    for (auto& [name, thread_map] : results) {
        std::cout << std::setw(25) << std::left << name;
        for (int num_threads : {1, 2, 3}) {
            double avg_time = thread_map[num_threads] / NUM_RUNS;
            std::cout << std::setw(10) << std::right << avg_time;
        }
        std::cout << std::endl;
    }
    std::cout << "----------------------------------------------------" << std::endl;
    return 0;
}