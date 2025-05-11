#include "Zipper/Unzipper.hpp"
#include <iostream>

#ifdef WIN32
#    include <windows.h>
#else
#    include <termios.h>
#    include <unistd.h>
#endif

#define DEFAULT_MAX_UNCOMPRESSED_SIZE_GB 6
#define TO_GB(bytes) (bytes * 1024 * 1024 * 1024)

// ----------------------------------------------------------------------------
static void usage(const char* argv0)
{
    std::cout << "Usage:\n  " << argv0
              << " [[-p] [-f] [-o path] [-m max_size] <path/to/file.zip>\n\n"
              << "Where:\n"
              << "  -p with AES password (from stdin)\n"
              << "  -o path to extract (default: .)\n"
              << "  -f force smashing files\n"
              << "  -m max uncompressed size in Giga bytes (default: "
              << DEFAULT_MAX_UNCOMPRESSED_SIZE_GB << ")\n"
              << std::endl;
}

// ----------------------------------------------------------------------------
//! \brief Quick and dirty parser the command-line option.
// ----------------------------------------------------------------------------
static std::string
cli(int argc, char* const argv[], const std::string& short_option)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == short_option)
        {
            if (i + 1 < argc)
                return argv[i + 1];
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
    std::string max_size_str = cli(argc, argv, "-m");
    size_t max_uncompressed_size = DEFAULT_MAX_UNCOMPRESSED_SIZE_GB;
    if (!max_size_str.empty())
    {
        try
        {
            max_uncompressed_size = std::stoull(max_size_str);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Invalid value for -m (max uncompressed size): '"
                      << max_size_str << "'\n";
            return EXIT_FAILURE;
        }
    }

    // Resume parsed options
    std::cout << "zip file: " << zip_file << "\n"
              << "extraction path: " << extraction_path << "\n"
              << "with password: " << with_password << "\n"
              << "force smashing files: " << force << "\n"
              << "max uncompressed size: " << max_uncompressed_size
              << " Giga bytes\n"
              << std::endl;

    // Check against zip file extension
    if (zip_file.substr(zip_file.find_last_of(".") + 1) != "zip")
    {
        std::cerr
            << "CLI error: Expected zip file on the last argument position"
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

        if (unzipper.sizeOnDisk() > TO_GB(max_uncompressed_size))
        {
            std::cerr
                << "Zip file uncompressed size exceeds the allowed limit ("
                << max_uncompressed_size
                << " GB). Use -m to set a different limit." << std::endl;
            return EXIT_FAILURE;
        }

        if (!unzipper.extractAll(
                extraction_path,
                force ? zipper::Unzipper::OverwriteMode::Overwrite
                      : zipper::Unzipper::OverwriteMode::DoNotOverwrite))
        {
            std::error_code err = unzipper.error();
            std::cerr << "Failed unzipping '" << zip_file << "' to '"
                      << (extraction_path == "" ? "." : extraction_path)
                      << "' Reason was: '" << err.message() << "'" << std::endl;
            return EXIT_FAILURE;
        }
        unzipper.close();
    }
    catch (std::runtime_error const& e)
    {
        std::cerr << "Failed unzipping '" << zip_file << "' Exception was: '"
                  << e.what() << "'" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[ok]" << std::endl;
    return EXIT_SUCCESS;
}
