#include "Zipper/Unzipper.hpp"
#include <iostream>

#ifdef WIN32
#  include <windows.h>
#else
#  include <termios.h>
#  include <unistd.h>
#endif

// ----------------------------------------------------------------------------
static void usage(const char* argv0)
{
    std::cout << "Usage:\n  " << argv0
              << " [[-p] [-f] [-o path] <path/to/file.zip>\n\n"
              << "Where:\n"
              << "  -p with AES password (from stdin)\n"
              << "  -o path to extract (default: .)\n"
              << "  -f force smashing files\n"
              << std::endl;
}

// ----------------------------------------------------------------------------
//! \brief Quick and dirty parser the command-line option.
// ----------------------------------------------------------------------------
static std::string cli(int argc, char* const argv[], const std::string& short_option)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == short_option)
        {
            if (i+1 < argc)
                return argv[i+1];
            return argv[i];
        }
    }
    return {};
}

// ----------------------------------------------------------------------------
//! \brief Extract a given zip file to an optional destination with an optional
//! password.
// ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Parse the command line
    if ((argc == 0) || (cli(argc, argv, "-h") != ""))
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string zip_file(argv[argc - 1]);
    std::string extraction_path(cli(argc, argv, "-o"));
    bool with_password(cli(argc, argv, "-p") != "");
    bool force(cli(argc, argv, "-f") != "");

    // Resume parsed options
    std::cout << "zip file: " << zip_file << "\n"
              << "extraction path: " << extraction_path << "\n"
              << "with password: " << with_password << "\n"
              << "force smashing files: " << force << "\n"
              << std::endl;

    // Check against zip file extension
    if (zip_file.substr(zip_file.find_last_of(".") + 1) != "zip")
    {
        std::cerr << "CLI error: Expected zip file on the last argument position"
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Read the password
    std::string password;
    if (with_password)
    {
        std::cout << "\nType your password: " << std::endl;
        std::cin >> password;
    }

    // libZipper functions
    try
    {
        zipper::Unzipper unzipper(zip_file, password);
        if (!unzipper.extractAll(extraction_path, force))
        {
            std::error_code err = unzipper.error();
            std::cerr << "Failed unzipping '" << zip_file
                      << "' to '"
                      << (extraction_path == "" ? "." : extraction_path)
                      << "' Reason was: '" << err.message()
                      << "'" << std::endl;
            return EXIT_FAILURE;
        }
        unzipper.close();
    }
    catch (std::runtime_error const& e)
    {
        std::cerr << "Failed unzipping '" << zip_file
                  << "' Exception was: '" << e.what()
                  << "'" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[ok]" << std::endl;
    return EXIT_SUCCESS;
}
