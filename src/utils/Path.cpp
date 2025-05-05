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
bool Path::isRoot(const std::string& p_path)
{
    // On Linux, we should recognize Windows root paths also
    if (p_path.length() == 1 && p_path[0] == UNIX_DIRECTORY_SEPARATOR)
        return true;

    // Recognize "C:\" or "C:/" as Windows roots even on Linux
    return (p_path.length() == 3) && (p_path[1] == ':') &&
           (((p_path[0] >= 'A') && (p_path[0] <= 'Z')) ||
            ((p_path[0] >= 'a') && (p_path[0] <= 'z'))) &&
           ((p_path[2] == UNIX_DIRECTORY_SEPARATOR) ||
            (p_path[2] == WINDOWS_DIRECTORY_SEPARATOR));
}

// -----------------------------------------------------------------------------
std::string Path::root(const std::string& p_path)
{
    // For Unix paths like "/path"
    if ((p_path.length() > 0) && (p_path[0] == UNIX_DIRECTORY_SEPARATOR))
        return STRING_PREFERRED_DIRECTORY_SEPARATOR;

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

    // Check whether the parent directory exists and is writable.
    if (!p_parent.empty() && (!isDir(p_parent) || !isWritable(p_parent)))
    {
        return false;
    }

    dir = normalize(dir);

    // ensure we have parent
    std::string actual_parent = dirName(dir);

    if (!actual_parent.empty() && (!exist(actual_parent)))
    {
        createDir(actual_parent);
    }

    return (OS_MKDIR(dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
}

#if 0
// -----------------------------------------------------------------------------
bool Path::removeFiles(const std::string& p_pattern,
                       const std::string& p_path)
{
    bool success = true;
    std::vector<std::string> p_pattern_list;

    p_pattern_list = compilePattern(p_pattern);

#    if defined(_WIN32)

    // We want the same pattern matching behaviour for all platforms.
    // Therefore, we do not use the MS provided one and list all files instead.
    std::string p_file_pattern = p_path + "\\*";

    // Open directory stream and try read info about first entry
    struct _finddata_t entry;
    intptr_t list = _findfirst(p_file_pattern.c_str(), &entry);

    if (list == -1)
        return success;

    do
    {
        std::string utf8 = entry.name;

        if (match(utf8, p_pattern_list))
        {
            if (entry.attrib | _A_NORMAL)
            {
                if (OS_REMOVE((p_path + WINDOWS_DIRECTORY_SEPARATOR + utf8).c_str()) != 0)
                    success = false;
            }
            else
            {
                if (OS_RMDIR((p_path + WINDOWS_DIRECTORY_SEPARATOR + utf8).c_str()) != 0)
                    success = false;
            }
        }
    } while (_findnext(list, &entry) == 0);

    _findclose(list);

#    else //! _WIN32

    DIR* dir = opendir(p_path.c_str());

    if (!dir) return false;

    struct dirent* entry;

    while ((entry = readdir(dir)) != nullptr)
    {
        std::string utf8 = entry->d_name;

        if (match(utf8, p_pattern_list))
        {
            if (isDir(utf8))
            {
                if (OS_RMDIR((p_path + UNIX_DIRECTORY_SEPARATOR + utf8).c_str()) != 0)
                    success = false;
            }
            else
            {
                if (OS_REMOVE((p_path + UNIX_DIRECTORY_SEPARATOR + utf8).c_str()) != 0)
                    success = false;
            }
        }
    }

    closedir(dir);

#    endif // _WIN32

    return success;
}
#endif

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

#if 0
// -----------------------------------------------------------------------------
bool Path::move(const std::string& p_from, const std::string& p_to)
{
    if (!isFile(p_from))
        return false;

    std::string to = p_to;

    // Check whether To is a directory and append the
    // filename of from
    if (isDir(to))
        to += STRING_PREFERRED_DIRECTORY_SEPARATOR + fileName(p_from);

    if (isDir(to))
        return false;

#    if defined(_WIN32)

    // The target must not exist under WIN32 for rename to succeed.
    if (exist(to) && !remove(to))
        return false;

#    endif // WIN32

    bool success = (::rename(p_from.c_str(), to.c_str()) == 0);

    if (!success)
    {
        {
            std::ifstream in(p_from.c_str());
            std::ofstream out(to.c_str());

            out << in.rdbuf();

            success = out.good();
        }

        OS_REMOVE(from.c_str());
    }

    return success;
}

// -----------------------------------------------------------------------------
std::vector<std::string> Path::compilePattern(const std::string& p_pattern)
{
    std::string::size_type pos = 0;
    std::string::size_type start = 0;
    std::string::size_type end = 0;
    std::vector<std::string> pattern_list;

    while (pos != std::string::npos)
    {
        start = pos;
        pos = p_pattern.find_first_of("*?", pos);

        end = std::min(pos, p_pattern.length());

        if (start != end)
        {
            pattern_list.push_back(p_pattern.substr(start, end - start));
        }
        else
        {
            pattern_list.push_back(p_pattern.substr(start, 1));
            pos++;
        }
    };

    return pattern_list;
}

// -----------------------------------------------------------------------------
bool Path::match(const std::string& p_name,
                 const std::vector<std::string>& p_pattern_list)
{
    std::vector<std::string>::const_iterator it = p_pattern_list.begin();
    std::vector<std::string>::const_iterator end = p_pattern_list.end();
    std::string::size_type at = 0;
    std::string::size_type after = 0;

    bool Match = true;

    while (it != end && Match)
    {
        Match = matchInternal(p_name, *it++, at, after);
    }

    return Match;
}
#endif

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

#if 0
// -----------------------------------------------------------------------------
bool Path::makePathRelative(std::string& p_absolute_path, const std::string& p_relative_to)
{
    if (isRelativePath(p_absolute_path) || isRelativePath(p_relative_to))
        return false; // Nothing can be done.

    std::string relative_to = normalize(p_relative_to);

    if (isFile(relative_to))
        relative_to = dirName(relative_to);

    if (!isDir(relative_to))
        return false;

    absolute_path = normalize(absolute_path);

    size_t i, imax = std::min(absolute_path.length(), relative_to.length());

    for (i = 0; i < imax; i++)
    {
        if (absolute_path[i] != relative_to[i])
            break;
    }

    // We need to retract to the beginning of the current directory.
    if (i != imax)
    {
        i = absolute_path.find_last_of('/', i) + 1;
    }

#    if defined(_WIN32)

    if (i == 0)
    {
        return false; // A different drive letter we cannot do anything
    }

#    endif // _WIN32

    relative_to = relative_to.substr(i);

    std::string relative_path;

    while (relative_to != "")
    {
        relative_path += "../";
        relative_to = dirName(relative_to);
    }

    if (relative_path != "")
    {
        absolute_path = relative_path + absolute_path.substr(i);
    }
    else
    {
        absolute_path = absolute_path.substr(i + 1);
    }

    return true;
}

// -----------------------------------------------------------------------------
bool Path::makePathAbsolute(std::string& p_relative_path, const std::string& p_absolute_to)
{
    if (!isRelativePath(p_relative_path) || isRelativePath(p_absolute_to))
        return false; // Nothing can be done.

    std::string absolute_to = normalize(p_absolute_to);

    if (isFile(absolute_to))
        absolute_to = dirName(absolute_to);

    if (!isDir(absolute_to))
        return false;

    relative_path = normalize(p_relative_path);

    while (!relative_path.compare(0, 3, "../"))
    {
        absolute_to = dirName(absolute_to);
        relative_path = relative_path.substr(3);
    }

    relative_path = absolute_to + DIRECTORY_SEPARATOR + relative_path;

    return true;
}

// -----------------------------------------------------------------------------
bool Path::matchInternal(const std::string& p_name,
                         const std::string& p_pattern,
                         std::string::size_type& p_at,
                         std::string::size_type& p_after)
{
    bool Match = true;

    switch (p_pattern[0])
    {
    case '*':
        if (p_at != std::string::npos)
        {
            p_after = p_at;
            p_at = std::string::npos;
        }
        break;

    case '?':
        if (p_at != std::string::npos)
        {
            ++p_at;
            Match = (p_name.length() >= p_at);
        }
        else
        {
            ++p_after;
            Match = (p_name.length() >= p_after);
        }
        break;

    default:
        if (p_at != std::string::npos)
        {
            Match = (p_name.compare(p_at, p_pattern.length(), p_pattern) == 0);
            p_at += p_pattern.length();
        }
        else
        {
            p_at = p_name.find(p_pattern, p_after);
            Match = (p_at != std::string::npos);
            p_at += p_pattern.length();
        }
        break;
    }

    return Match;
}
#endif

// -----------------------------------------------------------------------------
// TODO: should be replaced by canonicalPath ?
std::string Path::normalize(const std::string& p_path)
{
    if (p_path.empty())
        return {};

    // Detect the type of path (Windows or Unix)
    char preferred_separator = Path::preferredSeparator(p_path);

    // First, convert all separators to Unix format for normalization
    std::string clean_path = p_path;

    // Convert Windows separators to Unix separators for normalization
    for (size_t i = 0; i < clean_path.length(); ++i)
    {
        if (clean_path[i] == WINDOWS_DIRECTORY_SEPARATOR)
            clean_path[i] = UNIX_DIRECTORY_SEPARATOR;
    }

    // Remove leading './'
    while (!clean_path.compare(0, 2, "./"))
    {
        clean_path = clean_path.substr(2);
    }

    std::string::size_type pos = 1;
    while (true)
    {
        pos = clean_path.find("//", pos);
        if (pos == std::string::npos)
            break;

        clean_path.erase(pos, 1);
    }

    // Collapse '/./' to '/'
    pos = 0;

    while (true)
    {
        pos = clean_path.find("/./", pos);
        if (pos == std::string::npos)
            break;

        clean_path.erase(pos, 2);
    }

    // Collapse '[^/]+/../' to '/'
    std::string::size_type start = clean_path.length();

    while (true)
    {
        pos = clean_path.rfind("/../", start);
        if (pos == std::string::npos)
            break;

        start = clean_path.rfind('/', pos - 1);
        if (start == std::string::npos)
            break;

        if (!clean_path.compare(start, 4, "/../"))
            continue;

        clean_path.erase(start, pos - start + 3);
        start = clean_path.length();
    }

    if (clean_path.empty())
        return {};

    if ((clean_path.back() == UNIX_DIRECTORY_SEPARATOR) ||
        (clean_path.back() == WINDOWS_DIRECTORY_SEPARATOR))
    {
        clean_path.pop_back();
    }

    // Restore Windows separators if it was a Windows path
    if (preferred_separator == WINDOWS_DIRECTORY_SEPARATOR)
    {
        return toWindowsSeparators(clean_path);
    }
    else
    {
        return toUnixSeparators(clean_path);
    }
}

// -----------------------------------------------------------------------------
std::string Path::canonicalPath(const std::string& p_path)
{
    if (p_path.empty())
        return {};

    // Determine the original separator style and work internally with Unix
    // separators
    char original_separator = Path::preferredSeparator(p_path);
    std::string current_path = toUnixSeparators(p_path);

    std::string root;
    bool is_absolute = false;

    // Identify and separate the root part
    if (current_path.length() >= 1 && current_path[0] == '/')
    {
        is_absolute = true;
        root = "/";
        current_path = current_path.substr(1);
    }
    else if (current_path.length() >= 2 && current_path[1] == ':')
    {
        // Matches C:, D:, etc. Normalize to C:/ format internally
        is_absolute = true;
        root = current_path.substr(0, 2) + "/";
        current_path =
            (current_path.length() > 2) ? current_path.substr(3) : "";
    }
    // else: path is relative

    std::vector<std::string> segments_in;
    std::string segment;
    std::stringstream ss(current_path);

    // Split the path by '/'
    while (std::getline(ss, segment, '/'))
    {
        segments_in.push_back(segment);
    }

    // Handle paths ending with '/', which adds an empty segment unless the path
    // was just the root.
    if ((!current_path.empty()) && current_path.back() == '/')
    {
        if (segments_in.empty() || !segments_in.back().empty())
        {
            segments_in.push_back(""); // Represent the trailing slash
        }
    }

    std::vector<std::string> segments_out;
    for (const std::string& seg : segments_in)
    {
        // Check for empty segment added by trailing slash; ignore it for
        // canonicalization unless it's the only segment after root
        if (seg.empty() && !segments_out.empty())
        {
            // Ignore trailing slash representation (empty string) if it's not
            // the first segment
            if (&seg == &segments_in.back())
            {
                continue;
            }
        }

        if (seg == ".")
        {
            // Ignore "." segments. Handle "." or "./" case later if
            // segments_out remains empty.
            continue;
        }

        if (seg == "..")
        {
            if (!segments_out.empty() && segments_out.back() != "..")
            {
                // If not already at root or navigating up relative path, pop
                // the last segment
                segments_out.pop_back();
            }
            else if (!is_absolute)
            {
                // Path is relative, and we are at the start or already have
                // "..", so keep adding ".."
                segments_out.push_back("..");
            }
            // If absolute and segments_out is empty, ".." at root is ignored.
        }
        else
        {
            // Regular segment (including potentially empty segment if path was
            // like "//" or "C://")
            if (!seg.empty())
            {
                // Explicitly skip empty segments from input like "//"
                segments_out.push_back(seg);
            }
        }
    }

    // --- Reconstruct the canonical path ---
    std::string result;

    // Handle cases resulting in an empty stack
    if (segments_out.empty())
    {
        if (is_absolute)
        {
            result = root; // Should be "/" or "C:/"
        }
        else
        {
            // Relative path simplified to nothing (e.g., "foo/..", "./", ".")
            result = ".";
        }
    }
    else
    {
        // Join the processed segments
        std::string joined_segments;
        for (size_t i = 0; i < segments_out.size(); ++i)
        {
            joined_segments += segments_out[i];
            // Add separator unless it's the last segment
            if (i < segments_out.size() - 1)
            {
                joined_segments += "/";
            }
        }

        if (is_absolute)
        {
            // Check if root already ends with separator (it should: "/" or
            // "C:/")
            if (!root.empty() && root.back() == '/')
            {
                result = root + joined_segments;
            }
            else
            {
                // Should not happen with current root logic, but fallback just
                // in case
                result = root + "/" + joined_segments;
            }
        }
        else
        {
            result = joined_segments;
        }

        // Preserve leading "./" if original path started with it and result is
        // still relative
        bool original_starts_with_dot_slash =
            (p_path.length() >= 2 && p_path[0] == '.' &&
             (p_path[1] == '/' || p_path[1] == '\\'));
        bool result_is_relative =
            !is_absolute && (segments_out.empty() || segments_out[0] != "..");

        // Ensure result is not just "." before prepending "./"
        if (original_starts_with_dot_slash && result_is_relative &&
            result != ".")
        {
            // Avoid cases like "./.." becoming "./../.."
            if (result.length() < 2 || result.substr(0, 2) != "..")
            {
                result = "./" + result;
            }
        }
    }

    // Final conversion to the original preferred separator style
    if (original_separator == WINDOWS_DIRECTORY_SEPARATOR)
    {
        return toWindowsSeparators(result);
    }
    else
    {
        // Ensure Unix style result doesn't accidentally contain Windows
        // separators
        return toUnixSeparators(result);
    }
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
