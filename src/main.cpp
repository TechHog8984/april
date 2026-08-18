#include "classreader.hpp"
#include "codegenluau.hpp"
#include "jarloader.hpp"
#include "options.hpp"

#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <format>
#include <sstream>
#include <string>

[[noreturn]] void printUsage(int argc, char** argv) {
    std::cout << std::format("april by techhog\n"
        "usage: {} inputfile [options]\n"
        "options:\n"
        "  --verbose                  -  enable logging\n"
        "  -o=outputfile              -  path to the output file\n"
        "  --output=outputfile        -  path to the output file\n"
        "  --jar=jarfile              -  path to the jar\n"
        "  --output-folder=folder     -  path to the output folder for jar\n"
        "  --disable-codegen-asserts  -  disable codegen asserts. run twice to disable even more\n"
        "  --constant-patch=patch     -  patch a constant pool entry. this is used internally; don't touch if you don't know what you are doing\n"
        ,argc ? argv[0] : "april") << std::endl;
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

bool april_logging_enabled = false;
int disable_codegen_asserts = 0;

size_t constant_patch_count = 0;

int main(int argc, char** argv) {
    if (argc < 2)
        printUsage(argc, argv);

    int ret = 0;

    int inputfile_argc = 0;
    const char* output_file_path = nullptr;

    const char* output_folder_path = nullptr;
    const char* input_jar_path = nullptr;

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if (arg[0] == '-') {
            if (strncmp(arg, "--verbose", 9) == 0) {
                april_logging_enabled = true;
                continue;
            } else if (strncmp(arg, "--disable-codegen-asserts", 26) == 0) {
                disable_codegen_asserts++;
                continue;
            } else if (!handleRecordOption("--output-folder", arg)) {
                output_folder_path = arg;
                continue;
            } else if (!handleRecordOption("-o", arg) || !handleRecordOption("--output", arg)) {
                output_file_path = arg;
                continue;
            } else if (!handleRecordOption("--jar", arg)) {
                input_jar_path = arg;
                continue;
            } else if (!handleRecordOption("--constant-patch", arg)) {
                ConstantPatch patch;

                std::string argstr = arg;
                size_t commapos = argstr.find(',', 0);
                if (commapos == std::string::npos) {
                    std::cerr << "[ERROR]: constant-patch data did not contain a comma" << std::endl;
                    exit(1);
                }

                std::string indexstr(argstr.begin(), argstr.begin() + commapos);
                uint16_t index = std::stoi(indexstr);
                // TODO: verify within uint16_t bounds
                patch.index = index;

                std::string listindexstr(argstr.begin() + commapos + 1, argstr.end());
                int listindex = std::stoi(listindexstr);
                patch.list_index = listindex;

                size_t patch_index = constant_patch_count++;
                if (patch_index >= CONSTANT_PATCH_MAX) {
                    std::cerr << "[ERROR]: you are adding too many patches! the current limit is " << CONSTANT_PATCH_MAX << std::endl;
                    exit(1);
                }
                constant_patches[patch_index] = std::move(patch);
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

    if (inputfile_argc == 0 && input_jar_path == nullptr) {
        std::cerr << "[ERROR]: you must pass an input file (run with no arguments for help)" << std::endl;
        exit(1);
    }

    // TODO: stop memset everything just use {} (test it tho)

    if (input_jar_path) {
        if (output_folder_path == nullptr) {
            std::cerr << "[ERROR]: you must pass an output folder if you're using --jar (run with no arguments for help)" << std::endl;
            exit(1);
        }

        if (constant_patch_count) {
            std::cerr << "[ERROR]: you cannot provide constant patches if using a jar input (not a functional limitation I just am explicitly disallowing it until i find a use case)" << std::endl;
            exit(1);
        }

        auto map = readJar(input_jar_path);
        if (!map)
            exit(1);

        if (map->empty()) {
            std::cerr << "[ERROR]: jar contained no .class files" << std::endl;
            exit(1);
        }

        std::string path;
        path.reserve(strlen(output_folder_path) + 20);
        std::string folderpath;
        folderpath.reserve(path.capacity());
        std::string parentpath;
        parentpath.reserve(folderpath.capacity());

        for (const auto& pair : *map) {
            Class _class;
            std::memset(&_class, 0, sizeof(Class));

            std::string output;

            std::istringstream file(pair.second);
            if (!file) {
                std::cout << "failed to make istream of content for file " << pair.first << std::endl;
                break;
            }

            if (april_logging_enabled)
                std::cout << "doing the file: " << pair.first << std::endl;

            path.assign(output_folder_path);
            path.push_back('/');
            path.append(pair.first);

            folderpath.assign(path);
            auto pos = folderpath.rfind('/');
            if (pos == std::string::npos) {
                std::cerr << "[ERROR]: invalid folder path '" << folderpath << '\'' << std::endl;
                exit(1);
            }
            folderpath.erase(pos);

            int classret = readClassFile(file, _class);
            int luauret = 1;
            if (!classret) {
                parentpath.assign(folderpath);
                parentpath.erase(0, strlen(output_folder_path) + 1);
                luauret = generateLuau(_class, output, parentpath);
            }
            destroyClass(_class);

            if (april_logging_enabled)
                std::cout << "did the file: " << pair.first << std::endl;

            if (classret)
                break;

            {
            std::error_code ec;
            std::filesystem::create_directories(folderpath, ec);
            if (ec) {
                std::cerr << "[ERROR]: failed to create an output folder '" << folderpath << "': " << ec.message() << std::endl;
                exit(1);
            }
            }

            path.append(".luau");
            std::ofstream output_file(path);
            if (!output_file) {
                std::cerr << "[ERROR]: failed to open output file '" << path << '\'' << std::endl;
                exit(1);
            }

            output_file << output;

            if (luauret)
                break;
        }

        return 0;
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

    ret = generateLuau(_class, output);
    if (ret) goto RETURN;

    if (output_file_path) {
        std::ofstream output_file(output_file_path);
        if (!output_file) {
            std::cerr << "[ERROR] failed to open '" << output_file_path << '\'' << std::endl;
            goto RETURN;
        }
        output_file << output;
    } else
        std::cout << "\ncodegen:\n" << output << std::endl;

    RETURN:
    destroyClass(_class);

    return ret;
}
