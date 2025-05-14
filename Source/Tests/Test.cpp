#include <CmdLine/CmdLine.h>

int main(int argc, char** argv) {
    struct InFontCfg {
        std::string file      = "";
        std::string pt        = "12";
        std::string dpi       = "96,96";
        std::string range     = "32,128";
        bool        verbose   = false;
        std::string cache_dir = ".";
    };
    aby::util::CmdLine cmd;
    aby::util::CmdLine::Opts opts{
        .desc = "Cmdline utility for use of AbyssFreetype library",     
        .name = "AbyssFreetype",
        .cerr = std::cerr,         
        .help = true,        
        .term_colors = true,  
    };

    InFontCfg in_cfg;
    bool version;

    if (!cmd.opt("file", "Font file to load", &in_cfg.file, true)
       .opt("pt", "Requested point size of font (Default: '12')", &in_cfg.pt)
       .opt("dpi", "Dots per inch (Default: '96,96')", &in_cfg.dpi)
       .opt("range", "Character range to load (Default: '32,128')", &in_cfg.range)
       .opt("cache_dir", "Directory to output cached png and binary glyph to (Default '.')", &in_cfg.cache_dir)
       .flag("version", "Display version number and build info", &version, false, {"file"})
       .flag("v", "Enable verbose log messages", &in_cfg.verbose, false)
       .parse(argc, argv, opts))
    {
        return 1;
    }


}