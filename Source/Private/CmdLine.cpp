#include "CmdLine/CmdLine.h"
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


namespace aby::util {
    
    bool supports_ansi_seq() {
    #ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) return false;
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        return SetConsoleMode(hOut, dwMode) != 0;
    #else
        const char* term = std::getenv("TERM");
        return isatty(fileno(stdout)) && term && std::string(term) != "dumb";
    #endif
    }

    bool supports_utf8() {
    #ifdef _WIN32
        // Check for UTF-8 code page (65001) on Windows
        return GetConsoleOutputCP() == 65001;
    #else
        // Check if the terminal's locale supports UTF-8
        const char* lang = std::getenv("LANG");
        if (lang && std::string(lang).find("UTF-8") != std::string::npos) {
            return true;
        }
        return false;
    #endif
    }

    std::size_t calc_max_line_width(const std::vector<std::string>& lines) {
        size_t max_width = 0;
        for (const auto& line : lines) {
            if (line.size() > max_width) max_width = line.size();
        }
        return max_width;
    }

    std::pair<std::string, std::string> make_box_top_bottom(std::size_t max_width, const std::string& name, const std::string& color = "\033[1;37m") {
        std::string reset = "\033[0m";
        std::string top    = color + "┌";
        top.append("─");
        top.append(reset + "\033[1;33m" + name + "\033[0m" + color);
        for (std::size_t i = 0; i < (max_width + 2) - (name.size() + 1); i++) {
            top.append("─");
        }
        top.append("┐" + reset);
        std::string bottom = color + "└"; 
            for (std::size_t i = 0; i < max_width + 2; i++) {
            bottom.append("─");
        }
        bottom.append("┘" + reset);
        return { top, bottom };
    }

    void print_with_seq(std::ostream& os, const std::string& line, std::size_t max_width, std::string begin_seq = "", std::string end_seq = "", const std::string& box_color = "") {
        os << box_color << "│\033[0m ";
        os << begin_seq;
        os << line;
        os << end_seq;
        os << std::string(max_width - line.size(), ' ');
        os << box_color << " │\033[0m\n";
    }
    
    std::vector<std::string> make_lines(const std::string& desc, const std::vector<CmdLine::Arg>& args) {
        std::vector<std::string> lines;

        if (!desc.empty()) {
            lines.emplace_back(desc);
            lines.emplace_back("");  // extra spacing
        }

        // Step 1: Calculate max width of the [--arg] strings
        std::size_t arg_col_width = 0;
        for (const auto& arg : args) {
            std::size_t len = 6 + arg.arg.size(); // "  [--" + arg + "]"
            if (len > arg_col_width) arg_col_width = len;
        }

        // Step 2: Format each line with padded argument part
        for (const auto& arg : args) {
            std::string conv = arg.type == CmdLine::EArgType::OPT ? "--" : "-";
            std::string left = "  [" + conv + arg.arg + "]";
            left += std::string(arg_col_width - left.size(), ' ');
            lines.emplace_back(left + "  " + arg.desc); // double space between arg and desc
        }
        return lines;
    }

    void CmdLine::help(const Opts& opts, const Errors& errs) {
        bool prettify = supports_ansi_seq() && opts.term_colors;
        bool use_utf8 = supports_utf8();
        std::vector<std::string> lines = make_lines(opts.desc, m_Args);

        if (prettify && use_utf8) {
            // ---------------- HELP INFO ---------------- 
            static std::string reset      = "\033[0m";
            static std::string bold_cyan  = "\033[1;36m";
            static std::string bold_red   = "\033[1;31m";
            static std::string bold_green = "\033[1;32m";
            size_t max_width              = calc_max_line_width(lines);
            auto [top, bottom]            = make_box_top_bottom(max_width, opts.name, bold_green);

            opts.cerr << top << '\n';
            for (std::size_t i = 0; i < lines.size(); i++) {
                std::string& line = lines[i];
                if (i == 0) {
                    print_with_seq(opts.cerr, line, max_width, "\033[4m", reset, bold_green);
                } else if (auto pos = line.find("[-"); pos != std::string::npos) {
                    std::size_t start = line.find("-", pos); 
                    std::size_t end   = line.find(']', start);
                    if (start != std::string::npos && end != std::string::npos && end <= line.size()) {
                        line.insert(end, reset);
                        line.insert(start, bold_cyan);
                    }
                    print_with_seq(opts.cerr, line, max_width + bold_cyan.size() + reset.size(), "", "", bold_green);
                } else {
                    print_with_seq(opts.cerr, line, max_width, "", "", bold_green);
                }
            }
            opts.cerr << bottom << '\n';

            // ---------------- MISSING ARGS ----------------
            if (errs.missing_arg_ct != 0 || !errs.additional_errs.empty()) {
                std::tie(top, bottom) = make_box_top_bottom(max_width, "Errors", bold_red);
                opts.cerr << top << '\n';
                if (errs.missing_arg_ct != 0) {
                    print_with_seq(opts.cerr, errs.missing_args, max_width + ((bold_cyan.size() + reset.size()) * errs.missing_arg_ct), "", "", bold_red);
                }
                for (auto& err : errs.additional_errs) {
                    print_with_seq(opts.cerr, err, max_width + ((bold_cyan.size() + reset.size()) * 2) , "", "", bold_red);
                }
                opts.cerr << bottom << '\n';
            }
        } else {
            for (auto& line : lines) {
                opts.cerr << line << '\n';
            }
            opts.cerr << errs.missing_args << '\n';
            for (auto& err : errs.additional_errs) {
                opts.cerr << err << '\n';
            }
        }

        opts.cerr << '\n';
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
        bool prettify = supports_ansi_seq() && opts.term_colors;
        bool do_help = false;
        this->flag("h", "Display help information.", &do_help, false);
        std::vector<std::string> found(m_Args.size(), "");
        Errors errors = { "", 0, {} }; 
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
                                if (prettify) {
                                    errors.additional_errs.emplace_back(
                                        "Incorrect syntax for \033[1;36m--" + m_Args[arg].arg +
                                        "\033[0m: expected '\033[1;36m--" +m_Args[arg].arg + "\033[0m=VALUE'"
                                    );
                                } else {
                                    errors.additional_errs.emplace_back(
                                        "Incorrect syntax for --" + m_Args[arg].arg +
                                        ": expected '--" +m_Args[arg].arg + "=VALUE'"
                                    );
                                }
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
                if (prettify) {
                    errors.missing_args += "\033[1;36m" + conv + arg.arg + "\033[0m";
                } else {
                    errors.missing_args += conv + arg.arg;
                }
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