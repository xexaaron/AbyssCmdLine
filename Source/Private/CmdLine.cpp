#include "CmdLine/CmdLine.h"

#include <PrettyPrint/PrettyPrint.h>

#include <cstdlib>
#include <regex>
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
#else
    #include <unistd.h>
#endif
#include <utility>


namespace aby::util {

    void CmdLine::help(const Opts& opts, const Errors& errs) {
        // ---------------- HELP INFO ---------------- 
        std::string content = "  " +  opts.desc + "\n\n";
        std::size_t arg_col_width = 0;
        for (const auto& arg : m_Args) {
            std::size_t len = 6 + arg.arg.size();
            if (len > arg_col_width) arg_col_width = len;
        }
        for (const auto& arg : m_Args) {
            std::string conv = arg.type == CmdLine::EArgType::OPT ? "--" : "-";
            std::string left = " " + std::string(arg.req ? "\033[35m*\033[0m" : " ") +
                " \033[37m[\033[0m" + "\033[1;36m" + conv + arg.arg + "\033[0m" + "\033[37m]\033[0m";
            
            std::size_t vis_width = visual_width(left);
            if (arg_col_width > vis_width) {
                left += std::string(arg_col_width - vis_width + 1, ' ');
            }

            content += left + "  \033[4m\033[1;90m" + arg.desc + "\033[0m\n"; 
        }
        content += "\n";
        pretty_print(content, opts.name, Colors{.box = EColor::GREEN, .ctx = EColor::YELLOW});
        
        // ---------------- MISSING ARGS ----------------

        if (errs.missing_arg_ct != 0 || !errs.additional_errs.empty()) {
            std::string err_str = "  " + errs.missing_args + "\n";
            for (auto& line : errs.additional_errs) {
                err_str += "  " + line + "\n";
            }
            pretty_print(err_str, "Errors", Colors{.box = EColor::RED, .ctx = EColor::YELLOW});
        }
    }

    CmdLine& CmdLine::opt(std::string_view arg, std::string_view desc, std::string* result, bool req) {
        Arg a{ std::string(arg), std::string(desc), EArgType::OPT, req };
        a.result.string = result;
        m_Args.emplace_back(a);
        return *this;
    }

    CmdLine& CmdLine::flag(std::string_view arg, std::string_view desc, bool* result, bool req, const std::vector<std::string>& invalidates_req) {
        Arg a{ std::string(arg), std::string(desc), EArgType::FLAG, req, invalidates_req };
        a.result.boolean = result;
        m_Args.emplace_back(a);
        return *this;
    }

    bool CmdLine::parse(int argc, char** argv, const Opts& opts) {
        if (m_Args.empty()) {
            return true;
        }
        bool do_help = false;
        this->flag("h", "Display help information.", &do_help, false);
        std::vector<std::string> found(m_Args.size(), "");
        Errors errors;
        bool success = true;
        
        for (int i = 0; i < argc; i++) {
            std::string_view curr_arg = argv[i];

            for (std::size_t arg = 0; arg < m_Args.size(); arg++) {
                std::string call_conv_opt  = "--" + m_Args[arg].arg;
                std::string call_conv_flag = "-" + m_Args[arg].arg;

                if (curr_arg.starts_with(call_conv_opt) || curr_arg.starts_with(call_conv_flag)) {
                    found.push_back(m_Args[arg].arg);
                    switch (m_Args[arg].type) {
                        case EArgType::OPT: {
                            std::string equal_conv = call_conv_opt + "=";
                            if (curr_arg.starts_with(equal_conv)) {
                                *(m_Args[arg].result.string) = std::string(curr_arg.substr(equal_conv.size()));
                            }
                            else {
                                errors.additional_errs.emplace_back(
                                    "Incorrect syntax for \033[1;36m--" + m_Args[arg].arg +
                                    "\033[0m: expected '\033[1;36m--" +m_Args[arg].arg + "\033[0m=VALUE'"
                                );
                                success = false;
                            }
                            break;
                        }
                        case EArgType::FLAG:
                            *(m_Args[arg].result.boolean) = true;
                            if (!m_Args[arg].invalidates_req.empty()) {
                                for (auto& argument : m_Args) {
                                    for (const auto& invalidation : m_Args[arg].invalidates_req) {
                                        if (invalidation == argument.arg && invalidation != m_Args[arg].arg) {
                                            argument.req = false;
                                        }
                                    }
                                }
                            }
                            break;
                        default:
                            std::unreachable();
                    }
                }
            }
        }

        if (do_help) {
            help(opts, {});
            return false;
        }

        errors.missing_args = "Missing: [";
        bool first = true;
        for (auto& arg : m_Args) {
            bool found_current = false;
            for (auto& found_arg : found) {
                if (found_arg == arg.arg) {
                    found_current = true;
                    continue;
                }
            }
            if (!found_current && arg.req) {
                if (!first) {
                    errors.missing_args += ", ";
                }
                std::string conv = arg.type == EArgType::OPT ? "--" : "-";
                errors.missing_args += "\033[1;36m" + conv + arg.arg + "\033[0m";
                errors.missing_arg_ct++;
                success = false;
                first = false;
            }
        }
        errors.missing_args += "]";

        if (!success) {
            if (opts.help) {
                help(opts, errors);
            }
        }

        return success;
    }
}