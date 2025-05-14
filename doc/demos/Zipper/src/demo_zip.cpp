#include "Zipper/Zipper.hpp"
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>

#ifdef WIN32
#    include <windows.h>
#else
#    include <termios.h>
#    include <unistd.h>
#endif

#define PROGRESS_BAR_WIDTH 50

// ----------------------------------------------------------------------------
//! \brief Display the usage of the program
//! \param[in] argv0 Name of the program
// ----------------------------------------------------------------------------
static void usage(const char* argv0)
{
    std::cout << "Usage:\n  " << argv0
              << " [[-p] [-o path/to/output.zip] <path/to/folder>\n\n"
              << "Where:\n"
              << "  -p with AES password (from stdin)\n"
              << "  -o path to the zip file to create (default: ./output.zip)\n"
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
//! \brief Display a progress bar.
//! \param[in] progress Progress information.
// ----------------------------------------------------------------------------
static void displayProgress(const zipper::Zipper::Progress& progress)
{
    // Compute the global percentage
    float percent = 0.0f;
    if (progress.total_bytes > 0)
    {
        percent = float(progress.bytes_processed) /
                  float(progress.total_bytes) * 100.0f;
    }

    // Compute the number of characters to display
    size_t filled = size_t((percent / 100.0f) * PROGRESS_BAR_WIDTH);

    // Display the progress bar
    std::cout << "\r[";
    for (size_t i = 0; i < PROGRESS_BAR_WIDTH; ++i)
    {
        if (i < filled)
            std::cout << "=";
        else
            std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << percent << "% ";

    // Display the current file and the number of files
    std::cout << "(" << progress.files_compressed << "/" << progress.total_files
              << ") ";
    std::cout << progress.current_file;

    // Clean the line
    std::cout << std::string(20, ' ') << "\r" << std::flush;

    // If finished, go to the next line
    if (progress.status != zipper::Zipper::Progress::Status::InProgress)
    {
        std::cout << std::endl;
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
        std::cerr
            << "CLI error: The last argument must be a folder or file path"
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

        // Set the progress callback
        zipper.setProgressCallback(displayProgress);

        if (!zipper.add(folder_path, zipper::Zipper::Better))
        {
            std::error_code err = zipper.error();
            std::cerr << "Compression failed for '" << folder_path << "' to '"
                      << zip_file << "' Reason: '" << err.message() << "'"
                      << std::endl;
            return EXIT_FAILURE;
        }

        zipper.close();
    }
    catch (std::runtime_error const& e)
    {
        std::cerr << "Compression failed for '" << folder_path
                  << "' Exception: '" << e.what() << "'" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[ok]" << std::endl;
    return EXIT_SUCCESS;
}