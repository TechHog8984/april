#include "classreader.hpp"
#include "codegenluau.hpp"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <format>

[[noreturn]] void printUsage(int argc, char** argv) {
    std::cout << std::format("april by techhog"
        "usage: {} inputfile [options]\n"
        "options:\n"
        "  -o=outputfile        -  path to the output file\n"
        "  --output=outputfile  -  path to the output file",
        argc ? argv[0] : "april") << std::endl;
    exit(1);
}

int handleRecordOption(const char* option, const char*& arg, bool can_be_empty = false) {
    size_t option_length = std::strlen(option);

    if (std::strncmp(arg, option, option_length) != 0)
        return 1;

    if (std::strlen(arg) == option_length || arg[option_length] != '=') {
        std::cerr << "[ERROR]: " << option << " expects an equals sign" << std::endl;
        return 1;
    } else if (!can_be_empty && std::strlen(arg) < option_length + 2) {
        std::cerr << "[ERROR]: " << option << " expects a value after the equals sign" << std::endl;
        return 1;
    }

    arg += option_length + 1;
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2)
        printUsage(argc, argv);

    int ret = 0;

    int inputfile_argc = 0;
    const char* output_file_path = nullptr;

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (arg[0] == '-') {
            if (!handleRecordOption("-o", arg) || !handleRecordOption("--output", arg)) {
                output_file_path = arg;
                continue;
            }
            goto INVALID_ARG;
        } else if (inputfile_argc)
            goto INVALID_ARG;
        else
            inputfile_argc = i;

        continue;

        INVALID_ARG:
        std::cerr << "[ERROR]: invalid arg '" << arg << "'; run with no arguments for help (try " << argv[0] << ")"  << std::endl;
        exit(1);
    }

    std::ifstream input_file(argv[inputfile_argc], std::ios::binary);
    if (!input_file) {
        std::cerr << "[ERROR]: failed to open '" << argv[inputfile_argc] << '\'' << std::endl;
        exit(1);
    }

    Class _class;
    std::string output;

    std::memset(&_class, 0, sizeof(Class));

    ret = readClassFile(input_file, _class);
    if (ret) goto RETURN;

    output.reserve(input_file.tellg() / 2); // rough estimate

    ret = generateLuau(_class, output);
    if (ret) goto RETURN;

    if (output_file_path) {
        std::ofstream output_file(output_file_path);
        if (!output_file) {
            std::cerr << "[ERROR] failed ot open '" << output_file_path << '\'' << std::endl;
            goto RETURN;
        }
        output_file << output;
    } else
        std::cout << "\ncodegen:\n" << output << std::endl;

    RETURN:
    destroyClass(_class);

    return ret;
}
