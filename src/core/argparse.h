#ifdef _MSC_VER
#pragma once
#endif

#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

class ArgumentParser {
private:
    struct Arg {
        Arg(char shortName, const std::string &longName, const std::string &value, bool required,
            const std::string &help = "")
            : shortName{shortName}, longName{longName}, value{value}, required{required}, help{help} {}

        char shortName;
        std::string longName;
        std::string value;
        bool required;
        std::string help;
    };

public:
    static ArgumentParser &getInstance() {
        static ArgumentParser parser;
        return parser;
    }

    virtual ~ArgumentParser() {}

    void print() const {
        int maxlen = 0;
        for (const auto &v : values) {
            maxlen = std::max(maxlen, (int)v.first.length());
        }
        maxlen += 1;

        printf("***** Parameters *****\n");
        for (const auto &v : values) {
            printf("%*s : %s\n", maxlen, v.first.c_str(), v.second.c_str());
        }
        printf("\n");
    }

    template <typename T>
    void addArgument(const std::string &shortName, const std::string &longName, T value, bool required = false,
                     const std::string &help = "") {
        std::string v = std::to_string(value);
        addArgument<std::string>(shortName, longName, v, required, help);
    }

    bool parse(int argc, char **argv_) {
        std::vector<std::string> argv;
        for (int i = 1; i < argc; i++) {
            std::string arg(argv_[i]);
            int pos = arg.find('=');
            if (pos != std::string::npos) {
                argv.push_back(arg.substr(0, pos));
                argv.push_back(arg.substr(pos + 1));
            } else {
                argv.push_back(arg);
            }
        }

        appName = std::string(argv_[0]);
        appPath = fs::absolute(fs::path(appName.c_str())).string();
        for (int i = 0; i < argv.size(); i++) {
            if (!isValidShort(argv[i]) && !isValidLong(argv[i])) {
                fprintf(stderr, "[WARNING] Unparsed argument: \"%s\"\n", argv[i].c_str());
                continue;
            }

            std::string longName = "";
            if (isValidShort(argv[i])) {
                const auto &it = short2long.find(parseShort(argv[i]));
                if (it == short2long.cend()) {
                    fprintf(stderr, "[WARNING] Unknown flag %s!\n", argv[i].c_str());
                    continue;
                }
                longName = it->second;
            } else {
                longName = parseLong(argv[i]);
            }

            if (table.find(longName) != table.cend()) {
                values[longName] = std::string(argv[i + 1]);
                i += 1;
            }
        }

        // Check requirements
        bool success = true;
        for (const auto &arg : args) {
            if (arg.required) {
                const auto it = values.find(arg.longName);
                if (it == values.cend()) {
                    success = false;
                    fprintf(stderr, "Required argument \"%s\" is not specified!\n", arg.longName.c_str());
                }
            }
        }

        return success;
    }

    std::string helpText() {
        std::stringstream ss;
        ss << "[ usage ] " << appName << " ";

        // First, print required arguments.
        for (const auto &arg : args) {
            if (arg.required) {
                ss << ("--" + arg.longName) << " " << toUpper(arg.longName) << " ";
            }
        }

        // Then, print non-required arguments.
        for (const auto &arg : args) {
            if (!arg.required) {
                ss << "[" << ("--" + arg.longName) << " " << toUpper(arg.longName) << "] ";
            }
        }
        ss << std::endl;

        // Finally, output help for each argument
        for (const auto &arg : args) {
            ss << arg.longName << ": " << arg.help << std::endl;
        }

        return ss.str();
    }

    int getInt(const std::string &name) const {
        const auto &it = values.find(name);
        if (it == values.cend()) {
            fprintf(stderr, "Unknown name: %s!\n", name.c_str());
            return 0;
        }
        return std::atoi(it->second.c_str());
    }

    double getDouble(const std::string &name) const {
        const auto &it = values.find(name);
        if (it == values.cend()) {
            fprintf(stderr, "Unknown name: %s!\n", name.c_str());
            return 0.0;
        }
        return std::atof(it->second.c_str());
    }

    bool getBool(const std::string &name) const {
        const auto &it = values.find(name);
        if (it == values.cend()) {
            fprintf(stderr, "Unknown name: %s!\n", name.c_str());
            return false;
        }

        std::string value = it->second;
        if (value == "True" || value == "true" || value == "Yes" || value == "yes") {
            return true;
        }

        if (value == "False" || value == "false" || value == "No" || value == "no") {
            return false;
        }

        throw std::runtime_error("Cannot parse spacified option to bool!");
    }

    std::string getString(const std::string &name) const {
        const auto &it = values.find(name);
        if (it == values.cend()) {
            fprintf(stderr, "Unknown name: %s!\n", name.c_str());
            return "";
        }
        return it->second;
    }

    std::string getExecutableName() const {
        return appName;
    }

    std::string getExecutablePath() const {
        return appPath;
    }

private:
    // Private methods
    ArgumentParser() = default;
    ArgumentParser(const ArgumentParser &) = delete;
    ArgumentParser &operator=(const ArgumentParser &) = delete;

    static bool isValidShort(const std::string &name) { return name.length() == 2 && name[0] == '-'; }

    static bool isValidLong(const std::string &name) { return name.length() > 2 && name[0] == '-' && name[1] == '-'; }

    static char parseShort(const std::string &name) {
        if (name == "") {
            return '\0';
        }

        if (!isValidShort(name)) {
            throw std::runtime_error("Short name must be a single charactor with a hyphen!");
        }
        return name[1];
    }

    static std::string parseLong(const std::string &name) {
        if (!isValidLong(name)) {
            throw std::runtime_error("Long name must begin with two hyphens!");
        }
        return name.substr(2);
    }

    static std::string toUpper(const std::string &value) {
        std::string ret = value;
        std::transform(ret.begin(), ret.end(), ret.begin(), toupper);
        return ret;
    }

    // Private parameters
    std::string appName, appPath;
    std::vector<Arg> args;
    std::unordered_map<std::string, uint32_t> table;
    std::unordered_map<char, std::string> short2long;
    std::unordered_map<std::string, std::string> values;
};

// ---------------------------------------------------------------------------------------------------------------------
// Template specialization
// ---------------------------------------------------------------------------------------------------------------------

template <>
inline void ArgumentParser::addArgument<std::string>(const std::string &shortName, const std::string &longName,
                                                     std::string value, bool required, const std::string &help) {
    const char sname = parseShort(shortName);
    const std::string lname = parseLong(longName);
    short2long.insert(std::make_pair(sname, lname));
    table.insert(std::make_pair(lname, args.size()));
    args.emplace_back(sname, lname, value, required, help);
    if (!required) {
        values.insert(std::make_pair(lname, value));
    }
}

template <>
inline void ArgumentParser::addArgument<const char *>(const std::string &shortName, const std::string &longName,
                                                      const char *value, bool required, const std::string &help) {
    addArgument<std::string>(shortName, longName, std::string(value), required, help);
}

#endif  // _ARG_PARSER_H_
