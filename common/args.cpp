#include "common/args.h"
#include "common/version.h"
#include <cstring>
#include <getopt.h>
#include <iostream>

Args::Args(int argc, char *argv[])
{
    daemon_ = false;
    show_menu_ = true;
    cfg_file_name_ = "./config.json";
    pack_fee_ = 0;
    sign_fee_ = 0;
    ParseCommand(argc, argv);
}

void Args::ParseCommand(int argc, char *argv[])
{
    return;
    struct option long_options[] = {
        {"menu", no_argument, nullptr, 'm'},
        {"config", required_argument, nullptr, 'c'},
        {"daemon", no_argument, nullptr, 'd'},
        {"sign", required_argument, nullptr, 's'},
        {"pack", optional_argument, nullptr, 'p'},
        {"help", no_argument, nullptr, 'h'},
        {"version", no_argument, nullptr, 'v'},
        {nullptr, 0, nullptr, 0},
    };
    int opt = 0;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "mc:d:s:p:h?v", long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'm':
            if (0 == strcmp(optarg, "false"))
                show_menu_ = false;
            else
                show_menu_ = true;
            break;
        case 'c':
            cfg_file_name_ = optarg;
            break;
        case 'd':
            show_menu_ = false;
            daemon_ = true;
            break;
        case 's':
            sign_fee_ = atof(optarg);

            break;
        case 'p':
            pack_fee_ = atof(optarg);
            break;
        case 'h':
        case '?':
            fprintf(stderr, "\n%s [option]\n"
                            "  -m|--menu              Show console menu.\n"
                            "  -c|--config filename   Set configuration file (default: ./config.json).\n"
                            "  -d|--daemon            Show console menu, will overwrite -m options.\n"
                            "  -s|--sign fee          Set signature fee.\n"
                            "  -p|--pack fee          Set package fee\n"
                            "  -h|--help              This information.\n"
                            "  -v|--version           Display program version.\n",
                    argv[0]);
            exit(0);
        case 'v':
            std::cout << g_software_version << std::endl;
            exit(0);
        default:
            break;
        }
    }
}
