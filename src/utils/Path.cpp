// -----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
// -----------------------------------------------------------------------------

#include "utils/Path.hpp"
#include "utils/OS.hpp"

#include <chrono>
#include <fstream>
#include <iterator>
#include <numeric>
#include <random>
#include <sstream>
#include <vector>

using namespace zipper;

// The most common convention for ZIP archives is to use Unix separators
#define PREFERRED_DIRECTORY_SEPARATOR UNIX_DIRECTORY_SEPARATOR
const std::string STRING_PREFERRED_DIRECTORY_SEPARATOR =
    std::string(1, PREFERRED_DIRECTORY_SEPARATOR);
#define CONVERT_TO_PREFERRED_SEPARATORS(a) toUnixSeparators(a)

// -----------------------------------------------------------------------------
char Path::preferredSeparator(const std::string& path)
{
    // Detect the type of path (Windows or Unix)
    bool isWindowsPath =
        (path.length() > 1 && path[1] == ':') ||
        (path.length() > 1 && path[0] == '\\' && path[1] == '\\');
    return isWindowsPath ? WINDOWS_DIRECTORY_SEPARATOR
                         : UNIX_DIRECTORY_SEPARATOR;
}

// -----------------------------------------------------------------------------
std::string Path::currentPath()
{
    char buffer[1024u];
    return (OS_GETCWD(buffer, sizeof(buffer)) ? std::string(buffer)
                                              : std::string(""));
}

// -----------------------------------------------------------------------------
bool Path::isFile(const std::string& path)
{
    STAT st;

    if (stat(path.c_str(), &st) == -1)
        return false;

#if defined(_WIN32)
    return ((st.st_mode & S_IFREG) == S_IFREG);
#else
    return S_ISREG(st.st_mode);
#endif
}

// -----------------------------------------------------------------------------
bool Path::isDir(const std::string& path)
{
    STAT st;

    if (stat(path.c_str(), &st) == -1)
        return false;

#if defined(_WIN32)
    return ((st.st_mode & S_IFDIR) == S_IFDIR);
#else
    return S_ISDIR(st.st_mode);
#endif
}

// -----------------------------------------------------------------------------
// Do not use dirName()
// https://github.com/sebastiandev/zipper/issues/21
// -----------------------------------------------------------------------------
std::string Path::folderNameWithSeparator(const std::string& p_folder_path)
{
    if (p_folder_path.empty())
        return STRING_PREFERRED_DIRECTORY_SEPARATOR;

    bool end_by_slash = Path::hasTrailingSlash(p_folder_path);
    std::string folder_name(
        end_by_slash ? std::string(p_folder_path.begin(), --p_folder_path.end())
                     : p_folder_path);

    const std::string folder_with_separator =
        folder_name + STRING_PREFERRED_DIRECTORY_SEPARATOR;
    return folder_with_separator;
}

// -----------------------------------------------------------------------------
bool Path::exist(const std::string& p_path)
{
    STAT st;

    if (stat(p_path.c_str(), &st) == -1)
        return false;

#if defined(_WIN32)
    return ((st.st_mode & S_IFREG) == S_IFREG ||
            (st.st_mode & S_IFDIR) == S_IFDIR);
#else
    return (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode));
#endif
}

// -----------------------------------------------------------------------------
bool Path::isReadable(const std::string& p_path)
{
    return (access(p_path.c_str(), 0x4) == 0);
}

// -----------------------------------------------------------------------------
bool Path::isWritable(const std::string& p_path)
{
    return (access(p_path.c_str(), 0x2) == 0);
}

// -----------------------------------------------------------------------------
std::string Path::fileName(const std::string& p_path)
{
    // Search the last separator, whether it is Windows or Unix
    std::string::size_type start = p_path.find_last_of("/\\");

    if (start == std::string::npos)
    {
        start = 0;
    }
    else
    {
        start++; // We do not want the separator.
    }

    return p_path.substr(start);
}

// -----------------------------------------------------------------------------
std::string Path::root(const std::string& p_path)
{
    // For Unix paths like "/path"
    if ((p_path.length() > 0) && (p_path[0] == UNIX_DIRECTORY_SEPARATOR))
    {
        return std::string(1, p_path[0]);
    }

    // On Windows, root is "\\\\path"
    if (p_path.length() >= 2 && p_path[0] == WINDOWS_DIRECTORY_SEPARATOR &&
        p_path[1] == WINDOWS_DIRECTORY_SEPARATOR)
    {
        return p_path.substr(0, 2);
    }

    // For Windows paths like "C:\path" or "C:/path"
    if ((p_path.length() > 2) && (p_path[1] == ':') &&
        (((p_path[0] >= 'A') && (p_path[0] <= 'Z')) ||
         ((p_path[0] >= 'a') && (p_path[0] <= 'z'))) &&
        ((p_path[2] == WINDOWS_DIRECTORY_SEPARATOR) ||
         (p_path[2] == UNIX_DIRECTORY_SEPARATOR)))
    {
        return p_path.substr(0, 2) + WINDOWS_DIRECTORY_SEPARATOR;
    }

    return {};
}

// -----------------------------------------------------------------------------
bool Path::isRoot(const std::string& p_path)
{
    std::string r = Path::root(p_path);
    if (!r.empty() && p_path == r)
        return true;
    return false;
}

// -----------------------------------------------------------------------------
std::string Path::dirName(const std::string& p_path)
{
    if (p_path == ".")
        return "";

    if (p_path == "..")
        return "";

    if (Path::isRoot(p_path))
        return p_path;

    size_t pos = p_path.rfind(UNIX_DIRECTORY_SEPARATOR);
    size_t pos2 = p_path.rfind(WINDOWS_DIRECTORY_SEPARATOR);

    // Find the last separator, whether it is '/' or '\\'
    if (pos == std::string::npos)
        pos = pos2;
    else if (pos2 != std::string::npos && pos2 > pos)
        pos = pos2;

    if (pos != std::string::npos)
    {
        // Example "/usr"
        if (pos == 0)
            return STRING_PREFERRED_DIRECTORY_SEPARATOR;

        // Example "X:/foo"
        if ((pos == 2) && p_path[1] == ':')
            return root(p_path);

        // Example "regular/path" or "/regular/path"
        return p_path.substr(0, pos);
    }

    // single relative directory
    return "";
}

// -----------------------------------------------------------------------------
std::string Path::extension(const std::string& p_path)
{
    // Get the filename without the path
    std::string filename = fileName(p_path);

    // Find the last point in the filename
    std::string::size_type pos = filename.find('.');

    // If no point is found or it is at the beginning of the name (hidden file
    // under Unix)
    if (pos == std::string::npos)
        return {};

    // Return everything after the last point
    return filename.substr(pos + 1);
}

// -----------------------------------------------------------------------------
bool Path::createDir(const std::string& p_dir, const std::string& p_parent)
{
    std::string dir;

    if (!p_parent.empty())
    {
        dir = p_parent + STRING_PREFERRED_DIRECTORY_SEPARATOR;
    }

    dir += p_dir;

    // Check whether the directory already exists and is writable.
    if (isDir(dir) && isWritable(dir))
        return true;

    dir = normalize(dir);

    // Ensure we have parent
    std::string actual_parent = dirName(dir);

    // Check whether the parent directory exists and is writable
    if (!actual_parent.empty())
    {
        if (!exist(actual_parent))
        {
            if (!createDir(actual_parent))
            {
                // errno is already defined by the recursive call
                return false;
            }
        }
        else if (!isDir(actual_parent))
        {
            errno = ENOTDIR;
            return false;
        }
        else if (!isWritable(actual_parent))
        {
            errno = EACCES;
            return false;
        }
    }

    int result = OS_MKDIR(dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if (result != 0 && errno == 0)
    {
        // If OS_MKDIR fails but does not define errno, we define it
        errno = EACCES;
    }

    return (result == 0);
}

// -----------------------------------------------------------------------------
static bool private_remove(const std::string& p_path)
{
    if (Path::isDir(p_path))
        return OS_RMDIR(p_path.c_str()) == 0;

    if (Path::isFile(p_path))
        return OS_UNLINK(p_path.c_str()) == 0;

    return false;
}

// -----------------------------------------------------------------------------
void Path::removeDir(const std::string& p_foldername)
{
    if (!private_remove(p_foldername))
    {
        std::vector<std::string> files =
            Path::filesFromDir(p_foldername, false);
        std::vector<std::string>::iterator it = files.begin();
        for (; it != files.end(); ++it)
        {
            if (Path::isDir(*it) && *it != p_foldername)
            {
                Path::removeDir(*it);
            }
            else
            {
                private_remove(it->c_str());
            }
        }

        private_remove(p_foldername);
    }
}

// -----------------------------------------------------------------------------
bool Path::remove(const std::string& p_path)
{
    if (Path::isDir(p_path))
    {
        Path::removeDir(p_path.c_str());
        return true;
    }

    if (Path::isFile(p_path))
        return OS_UNLINK(p_path.c_str()) == 0;

    return false;
}

// -----------------------------------------------------------------------------
std::vector<std::string> Path::filesFromDir(const std::string& p_path,
                                            const bool p_recurse)
{
    std::vector<std::string> files;

#if defined(_WIN32)
    // For Windows, use the FindFirst/FindNext API
    std::string file_pattern = p_path + "\\*";
    struct _finddata_t entry;
    intptr_t list = _findfirst(file_pattern.c_str(), &entry);

    if (list == -1)
        return files;

    do
    {
        std::string filename(entry.name);

        if (filename == "." || filename == "..")
            continue;

        if (p_recurse)
        {
            if (Path::isDir(p_path + STRING_PREFERRED_DIRECTORY_SEPARATOR +
                            filename))
            {
                std::vector<std::string> moreFiles = Path::filesFromDir(
                    p_path + STRING_PREFERRED_DIRECTORY_SEPARATOR + filename,
                    p_recurse);
                std::copy(moreFiles.begin(),
                          moreFiles.end(),
                          std::back_inserter(files));
                continue;
            }
        }
        files.push_back(p_path + STRING_PREFERRED_DIRECTORY_SEPARATOR +
                        filename);
    } while (_findnext(list, &entry) == 0);

    _findclose(list);
#else
    DIR* dir;
    struct dirent* entry;

    dir = opendir(p_path.c_str());

    if (dir == nullptr)
        return files;

    for (entry = readdir(dir); entry != nullptr; entry = readdir(dir))
    {
        std::string filename(entry->d_name);

        if (filename == "." || filename == "..")
            continue;

        if (p_recurse)
        {
            if (Path::isDir(p_path + STRING_PREFERRED_DIRECTORY_SEPARATOR +
                            filename))
            {
                std::vector<std::string> moreFiles = Path::filesFromDir(
                    p_path + STRING_PREFERRED_DIRECTORY_SEPARATOR + filename,
                    p_recurse);
                std::copy(moreFiles.begin(),
                          moreFiles.end(),
                          std::back_inserter(files));
                continue;
            }
        }
        files.push_back(p_path + STRING_PREFERRED_DIRECTORY_SEPARATOR +
                        filename);
    }

    closedir(dir);
#endif

    return files;
}

// -----------------------------------------------------------------------------
std::string Path::getTempDirectory()
{
    std::string tempDir;

    // Windows
#ifdef _WIN32
    char tempPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tempPath) != 0)
    {
        tempDir = tempPath;
    }

    // Linux / macOS
#else
    const char* tmpDir = std::getenv("TMPDIR"); // Try TMPDIR (macOS)
    if (!tmpDir)
        tmpDir = std::getenv("TMP"); // Try TMP
    if (!tmpDir)
        tmpDir = std::getenv("TEMP"); // Try TEMP
    if (!tmpDir)
        tmpDir = "/tmp"; // Fallback default (Linux)

    tempDir = tmpDir;
#endif

    // Ensure the path ends with a separator
    if ((!tempDir.empty()) && (tempDir.back() != UNIX_DIRECTORY_SEPARATOR) &&
        (tempDir.back() != WINDOWS_DIRECTORY_SEPARATOR))
    {
        tempDir += DIRECTORY_SEPARATOR;
    }

    return tempDir;
}

// -----------------------------------------------------------------------------
std::string Path::createTempName(const std::string& p_dir,
                                 const std::string& p_suffix)
{
    std::mt19937 engine(static_cast<std::mt19937::result_type>(
        std::chrono::system_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(0, 35);
    std::string random_name;

    do
    {
        if ((!p_dir.empty()) && (p_dir.back() != UNIX_DIRECTORY_SEPARATOR) &&
            (p_dir.back() != WINDOWS_DIRECTORY_SEPARATOR))
        {
            random_name = p_dir + DIRECTORY_SEPARATOR;
        }
        else
        {
            random_name = p_dir;
        }
        int Char;

        for (size_t i = 0; i < 8u; i++)
        {
            Char = dist(engine);

            if (Char < 10)
            {
                random_name += char('0' + Char);
            }
            else
            {
                random_name += char('a' - 10 + Char);
            }
        }

        random_name += p_suffix;
    } while (exist(random_name));

    return random_name;
}

// -----------------------------------------------------------------------------
bool Path::isRelativePath(const std::string& p_path)
{
    std::string path = normalize(p_path);

    if (path.length() == 0)
        return false;

    if ((path[0] == UNIX_DIRECTORY_SEPARATOR ||
         path[0] == WINDOWS_DIRECTORY_SEPARATOR))
        return false;

    // Windows path with drive letter
    if (path.length() > 1 && path[1] == ':')
        return false;

    return true;
}

// -----------------------------------------------------------------------------
std::string Path::normalize(const std::string& p_path)
{
    if (p_path.empty())
        return {};

    std::string preferred_separator;
    std::string root;
    std::string current_path = toUnixSeparators(p_path);
    bool is_absolute = false;

#if 0
    // UNC path detection (Windows only: //server/share or \\\\server\\share)
    bool is_unc = false;
    if ((current_path.length() > 2) && (current_path[0] == '/') &&
        (current_path[1] == '/'))
    {
        size_t first = 2;
        size_t slash1 = current_path.find('/', first);
        if (slash1 != std::string::npos && slash1 > first)
        {
            size_t slash2 = current_path.find('/', slash1 + 1);
            if (slash2 != std::string::npos && slash2 > slash1 + 1)
            {
                // Cas UNC avec sous-dossier : //server/share/...
                is_unc = true;
                root = current_path.substr(0, slash2);
                current_path = current_path.substr(slash2 + 1);
                preferred_separator = "\\";
                is_absolute = true;
            }
            else if (slash1 + 1 < current_path.length())
            {
                // Cas UNC racine : //server/share
                is_unc = true;
                root = current_path;
                current_path.clear();
                preferred_separator = "\\";
                is_absolute = true;
            }
        }
    }
    if (!is_unc)
    {
#endif
    // Unix path
    if (current_path.length() >= 1 && current_path[0] == '/')
    {
        root = "/";
        preferred_separator = "/";
        current_path = current_path.substr(1);
        is_absolute = true;
    }
    // Windows path with drive letter (ex: C:)
    else if (current_path.length() >= 2 && current_path[1] == ':')
    {
        root = current_path.substr(0, 2) + "\\";
        preferred_separator = "\\";
        current_path =
            (current_path.length() > 2) ? current_path.substr(3) : "";
        is_absolute = true;
    }
    else
    {
        preferred_separator = std::string(1, Path::preferredSeparator(p_path));
    }
    // } // end of UNC path detection

    // Remove the initial "./" for relative paths
    if (!is_absolute && current_path.length() >= 2 && current_path[0] == '.' &&
        current_path[1] == '/')
    {
        current_path = current_path.substr(2);
    }

    // Split the path into segments
    std::vector<std::string> segments;
    std::string segment;
    std::stringstream ss(current_path);

    while (std::getline(ss, segment, '/'))
    {
        if (segment.empty() || segment == ".")
        {
            continue;
        }
        else if (segment == "..")
        {
            if (!segments.empty() && segments.back() != "..")
            {
                segments.pop_back();
            }
            else if (!is_absolute)
            {
                segments.push_back(segment);
            }
        }
        else
        {
            segments.push_back(segment);
        }
    }

    // Rebuild the normalized path using the preferred separator
    std::string result;

    if (segments.empty())
    {
        if (is_absolute)
        {
            result = root;
        }
        else
        {
            result = ".";
        }
    }
    else
    {
        if (is_absolute)
        {
            result = root;
        }
        for (size_t i = 0; i < segments.size(); ++i)
        {
            result += segments[i];
            if (i < segments.size() - 1)
            {
                result += preferred_separator;
            }
        }
    }

    return result;
}

// -----------------------------------------------------------------------------
bool Path::isLargeFile(std::istream& p_input_stream)
{
    std::streampos pos = 0;
    p_input_stream.seekg(0, std::ios::end);
    pos = p_input_stream.tellg();
    p_input_stream.seekg(0);

    return pos >= 0xffffffff;
}

// -----------------------------------------------------------------------------
bool Path::hasTrailingSlash(const std::string& p_path)
{
    return (p_path.size() >= 1u) &&
           (p_path.back() == WINDOWS_DIRECTORY_SEPARATOR ||
            p_path.back() == UNIX_DIRECTORY_SEPARATOR);
}

// -----------------------------------------------------------------------------
std::string Path::toZipArchiveSeparators(const std::string& p_path)
{
    return CONVERT_TO_PREFERRED_SEPARATORS(p_path);
}

// -----------------------------------------------------------------------------
bool Path::hasMixedSeparators(const std::string& p_path)
{
    bool hasWindowsSep = false;
    bool hasUnixSep = false;

    for (size_t i = 0; i < p_path.length(); ++i)
    {
        if (p_path[i] == WINDOWS_DIRECTORY_SEPARATOR)
            hasWindowsSep = true;
        else if (p_path[i] == UNIX_DIRECTORY_SEPARATOR)
            hasUnixSep = true;

        if (hasWindowsSep && hasUnixSep)
            return true;
    }

    return false;
}

// -----------------------------------------------------------------------------
std::string Path::toUnixSeparators(const std::string& p_path)
{
    std::string result = p_path;
    for (size_t i = 0; i < result.length(); ++i)
    {
        if (result[i] == WINDOWS_DIRECTORY_SEPARATOR)
            result[i] = UNIX_DIRECTORY_SEPARATOR;
    }
    return result;
}

// -----------------------------------------------------------------------------
std::string Path::toWindowsSeparators(const std::string& p_path)
{
    std::string result = p_path;
    for (size_t i = 0; i < result.length(); ++i)
    {
        if (result[i] == UNIX_DIRECTORY_SEPARATOR)
            result[i] = WINDOWS_DIRECTORY_SEPARATOR;
    }
    return result;
}

// -----------------------------------------------------------------------------
std::string Path::toNativeSeparators(const std::string& p_path)
{
#if defined(_WIN32)
    return toWindowsSeparators(p_path);
#else
    return toUnixSeparators(p_path);
#endif
}

// -----------------------------------------------------------------------------
size_t Path::getFileSize(const std::string& p_path)
{
    std::ifstream file(p_path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return 0; // Return 0 if file doesn't exist or cannot be opened
    }
    std::streampos size = file.tellg();
    file.close();
    return (size >= 0) ? static_cast<size_t>(size) : 0;
}

// -----------------------------------------------------------------------------
bool Path::isZipSlip(const std::string& p_file_path,
                     const std::string& p_destination_dir)
{
    std::string dest = p_destination_dir.empty() ? "." : p_destination_dir;
    dest = Path::normalize(dest);

    // Trailing slash is mandatory to avoid matching "john" and "johnny".
    // The if is important to avoid adding an extra trailing slash if
    // destination is a root path.
    if (!hasTrailingSlash(dest))
        dest += "/";

    // A slip slip attack uses '../' which will change the destination path.
    std::string file = Path::normalize(dest + p_file_path);
    return file.find(dest) != 0;
}