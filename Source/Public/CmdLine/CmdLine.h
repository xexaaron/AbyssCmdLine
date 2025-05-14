#pragma once
#include <string_view>
#include <string>
#include <vector>
#include <ostream>
#include <iostream>

namespace aby::util {

    class CmdLine {
    public:
        struct Opts {
            std::string   desc        = "";
            std::string   name        = "";
            std::ostream& cerr        = std::cerr;
            bool          help        = true; // Display help if failed to parse.
            bool          term_colors = true; // Use colored output if utf8 supported terminal.
        };
        enum class EArgType {
            OPT,
            FLAG
        };
        struct Arg {
            std::string arg = "";
            std::string desc = "";
            EArgType    type = EArgType::OPT;
            bool        req = false;
            std::vector<std::string> invalidates_req;
            union {
                std::string* string;
                bool* boolean;
            } result;
        };
        struct Errors {
            std::string missing_args;
            std::size_t missing_arg_ct;
            std::vector<std::string> additional_errs;
        };
    public:
        CmdLine& opt(std::string_view arg, std::string_view desc, std::string* result, bool req = false);
        CmdLine& flag(std::string_view arg, std::string_view desc, bool* result, bool req = false, const std::vector<std::string>& invalidates_req = {});
        void help(const Opts& opts = {}, const Errors& errs = {});
        bool parse(int argc, char** argv, const Opts& opts);
    private:
        std::vector<Arg> m_Args;
    };


}
