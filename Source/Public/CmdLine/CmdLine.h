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
            std::string   desc = "";
            std::ostream& cerr = std::cerr;
            bool          help = true; // Display help if failed to parse.
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
            union {
                std::string* string;
                bool* boolean;
            } result;
        };
    public:
        CmdLine& opt(std::string_view arg, std::string_view desc, std::string* result, bool req = false);
        CmdLine& flag(std::string_view arg, std::string_view desc, bool* result, bool req = false);
        void help(const Opts& opts = {});
        bool parse(int argc, char** argv, const Opts& opts);
    private:
        std::vector<Arg> m_Args;
    };


    CmdLine& CmdLine::opt(std::string_view arg, std::string_view desc, std::string* result, bool req) {
        Arg a{ std::string(arg), std::string(desc), EArgType::OPT, req };
        a.result.string = result;
        m_Args.emplace_back(a);
        return *this;
    }

    CmdLine& CmdLine::flag(std::string_view arg, std::string_view desc, bool* result, bool req) {
        Arg a{ std::string(arg), std::string(desc), EArgType::FLAG, req };
        a.result.boolean = result;
        m_Args.emplace_back(a);
        return *this;
    }

    void CmdLine::help(const Opts& opts) {
        if (!opts.desc.empty()) {
            opts.cerr << opts.desc << "\n\n";
        }
        for (auto& arg : m_Args) {
            opts.cerr << "\t[--" << arg.arg << "]\t" << arg.desc << '\n';
        }
        opts.cerr << '\n';
    }

    bool CmdLine::parse(int argc, char** argv, const Opts& opts) {
        if (m_Args.empty()) {
            return true;
        }
        
        std::vector<bool> found(m_Args.size(), false);

        for (int i = 0; i < argc; i++) {
            std::string_view curr_arg = argv[i];

            for (std::size_t arg = 0; arg < m_Args.size(); arg++) {
                std::string call_conv = "--" + m_Args[arg].arg;

                if (curr_arg.starts_with(call_conv)) {
                    found[arg] = true;
                    switch (m_Args[arg].type) {
                    case EArgType::OPT: {
                        std::string equal_conv = call_conv + "=";
                        if (curr_arg.starts_with(equal_conv)) {
                            *(m_Args[arg].result.string) = std::string(curr_arg.substr(equal_conv.size()));
                        }
                        else {
                            opts.cerr << "Incorrect syntax for --" << m_Args[arg].arg
                                << ": expected '--" << m_Args[arg].arg << "=VALUE'\n";
                        }
                        break;
                    }
                    case EArgType::FLAG:
                        *(m_Args[arg].result.boolean) = true;
                        break;
                    default:
                        std::unreachable();
                    }
                }
            }
        }

        bool success = true;
        std::string err = "Missing: [";
        bool first = true;

        for (std::size_t i = 0; i < m_Args.size(); i++) {
            if (!found[i] && m_Args[i].req) {
                if (!first) {
                    err += ", ";
                }
                err += "--" + m_Args[i].arg;
                success = false;
                first = false;
            }
        }
        err += "]";

        if (!success) {
            if (opts.help) {
                help(opts);
            }
            opts.cerr << err << '\n';
        }

        return success;
    }
}
