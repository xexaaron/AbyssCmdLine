#include "CmdLine/CmdLine.h"

#include <PrettyPrint/PrettyPrint.h>
#include <filesystem>

#include <cstdlib>
#include <regex>
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
    #undef max
#else
    #include <unistd.h>
#endif
#include <utility>


#define TIME_CMDLINE

#if !defined(NDEBUG) && defined(TIME_CMDLINE)
    #include <chrono>

    #define TIMER_START(...) \
        auto timer_start = std::chrono::high_resolution_clock::now();

    #define TIMER_END(ctx) \
        auto timer_end = std::chrono::high_resolution_clock::now(); \
        auto timer_diff = std::chrono::duration_cast<std::chrono::milliseconds>(timer_end - timer_start).count(); \
        pretty_print(std::format("Time elapsed: {} ms", timer_diff), ctx, Colors{EColor::CYAN, EColor::MAGENTA});
#else
    #define TIMER_START(...)
    #define TIMER_END(...)
#endif

namespace aby::util {

    void CmdLine::help(const Opts& opts, const Errors& errs) {
        TIMER_START();

        // ------------ HELP HEADER ------------
        std::string content;
        content.reserve(2048); 
        content += "  " + opts.desc + "\n\n";

        std::size_t arg_col_width = 0;
        for (const auto& arg : m_Args) {
            arg_col_width = std::max(arg_col_width, 6 + arg.arg.size());
        }

        // ------------ ARGUMENT LIST ------------
        for (const auto& arg : m_Args) {
            const char* conv = (arg.type == CmdLine::EArgType::OPT) ? "--" : "-";
            const char* required = arg.req ? "\033[35m*\033[0m" : " ";
            
            std::string left = " ";
            left += required;
            left += " \033[37m[\033[0m\033[1;36m";
            left += conv;
            left += arg.arg;
            left += "\033[0m\033[37m]\033[0m";

            std::size_t vis_width = visual_width(left);
            if (arg_col_width > vis_width) {
                left.append(arg_col_width - vis_width + 1, ' ');
            }

            content += left;
            content += "  \033[4m\033[1;90m";
            content += arg.desc;
            content += "\033[0m\n";
        }
        content += '\n';

        pretty_print(content, opts.name, Colors{.box = EColor::GREEN, .ctx = EColor::YELLOW});

        // ------------ ERRORS ------------
        if (errs.missing_arg_ct != 0 || !errs.additional_errs.empty()) {
            std::string err_str;
            err_str.reserve(512);  

            if (errs.missing_arg_ct != 0) {
                err_str += "  " + errs.missing_args + '\n';
            }
            for (const auto& line : errs.additional_errs) {
                err_str += "  " + line + '\n';
            }
            pretty_print(err_str, "Errors", Colors{.box = EColor::RED, .ctx = EColor::YELLOW});
        }

        TIMER_END("Help");
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
        TIMER_START("Parse");
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

                if (curr_arg == call_conv_opt || curr_arg == call_conv_flag) {
                    found.push_back(m_Args[arg].arg);

                    switch (m_Args[arg].type) {
                        case EArgType::OPT: {
                            if (i + 1 < argc) {
                                *(m_Args[arg].result.string) = argv[++i];  // consume next arg as value
                            } else {
                                errors.additional_errs.emplace_back(
                                    "Missing value for \033[1;36m" + call_conv_opt + "\033[0m."
                                );
                                success = false;
                            }
                            break;
                        }

                        case EArgType::FLAG:
                            if (curr_arg.starts_with("--")) {
                                 errors.additional_errs.emplace_back(
                                    "Incorrect syntax for flag. Expected \033[1;36m" + call_conv_flag + "\033[0m" + " but got \033[1;36m-" + call_conv_flag + "\033[0m." 
                                );
                                success = false;
                            }
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
            if (opts.log_cmd) {
                log_command(argv[0], found);
            }
            return false;
        }

        find_missing(found, errors, success);

        if (!success) {
            if (opts.help) {
                help(opts, errors);
            }
        }

        if (opts.log_cmd) {
            log_command(argv[0], found);
        }

        TIMER_END("Parse");



        return success;
    }

    
    bool CmdLine::is_number(const std::string& string) const {
        for (char c : string) {
            if (!std::isdigit(c)) {
                return false;
            }
        }
        return true;
    }

    void CmdLine::find_missing(const std::vector<std::string>& found, Errors& errors, bool& success) {
        TIMER_START("Find Missing");
        errors.missing_args = "Missing: [";
        bool first = true;
        for (auto& arg : m_Args) {
            bool found_current = false;
            for (auto& found_arg : found) {
                if (found_arg == arg.arg) {
                    found_current = true;
                    break;
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
        TIMER_END("Find Missing");
    }

    void CmdLine::log_command(const std::string& program_name, const std::vector<std::string>& found) {
        TIMER_START("Log Command");
        std::string log_cmd = "  \033[33m" + std::filesystem::path(program_name).filename().string() + "\033[0m" + " ";
        for (auto& arg : m_Args) {
            bool log_arg = false;
            for (auto& found_arg : found) {
                if (found_arg == arg.arg) {
                    log_arg = true;
                    break;
                }
            }
            if (log_arg) {
                if (arg.type == EArgType::FLAG) {
                    log_cmd += "\033[1;36m-" + arg.arg + "\033[0m" + " ";
                } else if (arg.type == EArgType::OPT) {
                    log_cmd += "\033[1;36m--" + arg.arg + "\033[0m" + " ";
                    log_cmd += *arg.result.string + " ";
                }
            }
            
        }
        aby::util::pretty_print(log_cmd, "Command", Colors{
            .box = EColor::MAGENTA,
        });
        TIMER_END("Log Command");
    }

}