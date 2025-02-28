#include "Zipper/Zipper.hpp"
#include <iostream>
#include <filesystem>

#ifdef WIN32
#  include <windows.h>
#else
#  include <termios.h>
#  include <unistd.h>
#endif

// ----------------------------------------------------------------------------
//! \brief Display the usage of the program
//! \param[in] argv0 Name of the program
// ----------------------------------------------------------------------------
static void usage(const char* argv0)
{
    std::cout << "Usage:\n  " << argv0
              << " [[-p] [-r] [-o path/to/output.zip] <path/to/folder>\n\n"
              << "Where:\n"
              << "  -p with AES password (from stdin)\n"
              << "  -r recursive compression of the folder\n"
              << "  -o path to the zip file to create (default: ./output.zip)\n"
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
//! \brief Add a file or a folder to the zip archive
//! \param[in,out] zipper Instance of Zipper
//! \param[in] path Path of the file or folder to add
//! \param[in] recursive If true, add the subfolders recursively
//! \return true if success, false otherwise
// ----------------------------------------------------------------------------
static bool addToZip(zipper::Zipper& zipper, const std::string& path, bool recursive)
{
    // Check if the path is a folder
    if (std::filesystem::is_directory(path))
    {
        if (recursive)
        {
            // Add the folder and its content recursively
            return zipper.add(path, zipper::Zipper::Better);
        }
        else
        {
            // Add only the files of the folder (without recursion)
            bool success = true;
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if (entry.is_regular_file())
                {
                    success &= zipper.add(entry.path().string(), zipper::Zipper::Better);
                }
            }
            return success;
        }
    }
    else
    {
        // Add a simple file
        return zipper.add(path, zipper::Zipper::Better);
    }
}

// ----------------------------------------------------------------------------
//! \brief Compress a given folder into a zip file with an optional password
// ----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Analyze the command-line
    if ((argc < 2) || (cli(argc, argv, "-h") != ""))
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string folder_path(argv[argc - 1]);
    std::string zip_file(cli(argc, argv, "-o"));
    bool with_password(cli(argc, argv, "-p") != "");
    bool recursive(cli(argc, argv, "-r") != "");

    // Default value for the zip file if not specified
    if (zip_file.empty())
    {
        zip_file = "output.zip";
    }

    // Summary of the analyzed options
    std::cout << "folder to compress: " << folder_path << "\n"
              << "zip file: " << zip_file << "\n"
              << "with password: " << with_password << "\n"
              << "recursive compression: " << recursive << "\n"
              << std::endl;

    // Check if the last argument is a valid path
    if (folder_path.empty() || folder_path[0] == '-')
    {
        std::cerr << "CLI error: The last argument must be a folder or file path"
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Read the password
    std::string password;
    if (with_password)
    {
        std::cout << "\nEnter your password: " << std::endl;
        std::cin >> password;
    }

    // libZipper functions
    try
    {
        zipper::Zipper zipper(zip_file, password, zipper::Zipper::Overwrite);

        if (!addToZip(zipper, folder_path, recursive))
        {
            std::error_code err = zipper.error();
            std::cerr << "Compression failed for '" << folder_path
                      << "' to '"
                      << zip_file
                      << "' Reason: '" << err.message()
                      << "'" << std::endl;
            return EXIT_FAILURE;
        }

        zipper.close();
    }
    catch (std::runtime_error const& e)
    {
        std::cerr << "Compression failed for '" << folder_path
                  << "' Exception: '" << e.what()
                  << "'" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[ok]" << std::endl;
    return EXIT_SUCCESS;
}