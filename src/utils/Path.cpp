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
std::string Path::folderNameWithSeparator(const std::string& folder_path)
{
    if (folder_path.empty())
        return STRING_PREFERRED_DIRECTORY_SEPARATOR;

    bool end_by_slash = Path::hasTrailingSlash(folder_path);
    std::string folder_name(
        end_by_slash ? std::string(folder_path.begin(), --folder_path.end())
                     : folder_path);

    const std::string folder_with_separator =
        folder_name + STRING_PREFERRED_DIRECTORY_SEPARATOR;
    return folder_with_separator;
}

// -----------------------------------------------------------------------------
bool Path::exist(const std::string& path)
{
    STAT st;

    if (stat(path.c_str(), &st) == -1)
        return false;

#if defined(_WIN32)
    return ((st.st_mode & S_IFREG) == S_IFREG ||
            (st.st_mode & S_IFDIR) == S_IFDIR);
#else
    return (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode));
#endif
}

// -----------------------------------------------------------------------------
bool Path::isReadable(const std::string& path)
{
    return (access(path.c_str(), 0x4) == 0);
}

// -----------------------------------------------------------------------------
bool Path::isWritable(const std::string& path)
{
    return (access(path.c_str(), 0x2) == 0);
}

// -----------------------------------------------------------------------------
std::string Path::fileName(const std::string& path)
{
    // Search the last separator, whether it is Windows or Unix
    std::string::size_type start = path.find_last_of("/\\");

    if (start == std::string::npos)
    {
        start = 0;
    }
    else
    {
        start++; // We do not want the separator.
    }

    return path.substr(start);
}

// -----------------------------------------------------------------------------
bool Path::isRoot(const std::string& path)
{
    // On Linux, we should recognize Windows root paths also
    if (path.length() == 1 && path[0] == UNIX_DIRECTORY_SEPARATOR)
        return true;

    // Recognize "C:\" or "C:/" as Windows roots even on Linux
    return (path.length() == 3) && (path[1] == ':') &&
           (((path[0] >= 'A') && (path[0] <= 'Z')) ||
            ((path[0] >= 'a') && (path[0] <= 'z'))) &&
           ((path[2] == UNIX_DIRECTORY_SEPARATOR) ||
            (path[2] == WINDOWS_DIRECTORY_SEPARATOR));
}

// -----------------------------------------------------------------------------
std::string Path::root(const std::string& path)
{
    // For Unix paths like "/path"
    if ((path.length() > 0) && (path[0] == UNIX_DIRECTORY_SEPARATOR))
        return STRING_PREFERRED_DIRECTORY_SEPARATOR;

    // For Windows paths like "C:\path" or "C:/path"
    if ((path.length() > 2) && (path[1] == ':') &&
        (((path[0] >= 'A') && (path[0] <= 'Z')) ||
         ((path[0] >= 'a') && (path[0] <= 'z'))) &&
        ((path[2] == WINDOWS_DIRECTORY_SEPARATOR) ||
         (path[2] == UNIX_DIRECTORY_SEPARATOR)))
    {
        return path.substr(0, 2) + WINDOWS_DIRECTORY_SEPARATOR;
    }

    return {};
}

// -----------------------------------------------------------------------------
std::string Path::dirName(const std::string& path)
{
    if (path == ".")
        return "";

    if (path == "..")
        return "";

    if (Path::isRoot(path))
        return path;

    size_t pos = path.rfind(UNIX_DIRECTORY_SEPARATOR);
    size_t pos2 = path.rfind(WINDOWS_DIRECTORY_SEPARATOR);

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
        if ((pos == 2) && path[1] == ':')
            return root(path);

        // Example "regular/path" or "/regular/path"
        return path.substr(0, pos);
    }

    // single relative directory
    return "";
}

// -----------------------------------------------------------------------------
std::string Path::extension(const std::string& path)
{
    // Get the filename without the path
    std::string filename = fileName(path);

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
bool Path::createDir(const std::string& dir, const std::string& parent)
{
    std::string Dir;

    if (!parent.empty())
    {
        Dir = parent + STRING_PREFERRED_DIRECTORY_SEPARATOR;
    }

    Dir += dir;

    // Check whether the directory already exists and is writable.
    if (isDir(Dir) && isWritable(Dir))
        return true;

    // Check whether the parent directory exists and is writable.
    if (!parent.empty() && (!isDir(parent) || !isWritable(parent)))
        return false;

    Dir = normalize(Dir);

    // ensure we have parent
    std::string actualParent = dirName(Dir);

    if (!actualParent.empty() && (!exist(actualParent)))
    {
        createDir(actualParent);
    }

    return (OS_MKDIR(Dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
}

#if 0
// -----------------------------------------------------------------------------
bool Path::removeFiles(const std::string& pattern,
                       const std::string& path)
{
    bool success = true;
    std::vector<std::string> PatternList;

    PatternList = compilePattern(pattern);

#    if defined(_WIN32)

    // We want the same pattern matching behaviour for all platforms.
    // Therefore, we do not use the MS provided one and list all files instead.
    std::string FilePattern = path + "\\*";

    // Open directory stream and try read info about first entry
    struct _finddata_t Entry;
    intptr_t hList = _findfirst(FilePattern.c_str(), &Entry);

    if (hList == -1)
        return success;

    do
    {
        std::string Utf8 = Entry.name;

        if (match(Utf8, PatternList))
        {
            if (Entry.attrib | _A_NORMAL)
            {
                if (OS_REMOVE((path + WINDOWS_DIRECTORY_SEPARATOR + Utf8).c_str()) != 0)
                    success = false;
            }
            else
            {
                if (OS_RMDIR((path + WINDOWS_DIRECTORY_SEPARATOR + Utf8).c_str()) != 0)
                    success = false;
            }
        }
    } while (_findnext(hList, &Entry) == 0);

    _findclose(hList);

#    else //! _WIN32

    DIR* pDir = opendir(path.c_str());

    if (!pDir) return false;

    struct dirent* pEntry;

    while ((pEntry = readdir(pDir)) != nullptr)
    {
        std::string Utf8 = pEntry->d_name;

        if (match(Utf8, PatternList))
        {
            if (isDir(Utf8))
            {
                if (OS_RMDIR((path + UNIX_DIRECTORY_SEPARATOR + Utf8).c_str()) != 0)
                    success = false;
            }
            else
            {
                if (OS_REMOVE((path + UNIX_DIRECTORY_SEPARATOR + Utf8).c_str()) != 0)
                    success = false;
            }
        }
    }

    closedir(pDir);

#    endif // _WIN32

    return success;
}
#endif

// -----------------------------------------------------------------------------
static bool private_remove(const std::string& path)
{
    if (Path::isDir(path))
        return OS_RMDIR(path.c_str()) == 0;

    if (Path::isFile(path))
        return OS_UNLINK(path.c_str()) == 0;

    return false;
}

// -----------------------------------------------------------------------------
void Path::removeDir(const std::string& foldername)
{
    if (!private_remove(foldername))
    {
        std::vector<std::string> files = Path::filesFromDir(foldername, false);
        std::vector<std::string>::iterator it = files.begin();
        for (; it != files.end(); ++it)
        {
            if (Path::isDir(*it) && *it != foldername)
            {
                Path::removeDir(*it);
            }
            else
            {
                private_remove(it->c_str());
            }
        }

        private_remove(foldername);
    }
}

// -----------------------------------------------------------------------------
bool Path::remove(const std::string& path)
{
    if (Path::isDir(path))
    {
        Path::removeDir(path.c_str());
        return true;
    }

    if (Path::isFile(path))
        return OS_UNLINK(path.c_str()) == 0;

    return false;
}

// -----------------------------------------------------------------------------
std::vector<std::string> Path::filesFromDir(const std::string& path,
                                            const bool recurse)
{
    std::vector<std::string> files;

#if defined(_WIN32)
    // For Windows, use the FindFirst/FindNext API
    std::string FilePattern = path + "\\*";
    struct _finddata_t Entry;
    intptr_t hList = _findfirst(FilePattern.c_str(), &Entry);

    if (hList == -1)
        return files;

    do
    {
        std::string filename(Entry.name);

        if (filename == "." || filename == "..")
            continue;

        if (recurse)
        {
            if (Path::isDir(path + STRING_PREFERRED_DIRECTORY_SEPARATOR +
                            filename))
            {
                std::vector<std::string> moreFiles = Path::filesFromDir(
                    path + STRING_PREFERRED_DIRECTORY_SEPARATOR + filename,
                    recurse);
                std::copy(moreFiles.begin(),
                          moreFiles.end(),
                          std::back_inserter(files));
                continue;
            }
        }
        files.push_back(path + STRING_PREFERRED_DIRECTORY_SEPARATOR + filename);
    } while (_findnext(hList, &Entry) == 0);

    _findclose(hList);
#else
    DIR* dir;
    struct dirent* entry;

    dir = opendir(path.c_str());

    if (dir == nullptr)
        return files;

    for (entry = readdir(dir); entry != nullptr; entry = readdir(dir))
    {
        std::string filename(entry->d_name);

        if (filename == "." || filename == "..")
            continue;

        if (recurse)
        {
            if (Path::isDir(path + STRING_PREFERRED_DIRECTORY_SEPARATOR +
                            filename))
            {
                std::vector<std::string> moreFiles = Path::filesFromDir(
                    path + STRING_PREFERRED_DIRECTORY_SEPARATOR + filename,
                    recurse);
                std::copy(moreFiles.begin(),
                          moreFiles.end(),
                          std::back_inserter(files));
                continue;
            }
        }
        files.push_back(path + STRING_PREFERRED_DIRECTORY_SEPARATOR + filename);
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
std::string Path::createTempName(const std::string& dir,
                                 const std::string& suffix)
{
    std::mt19937 engine(static_cast<std::mt19937::result_type>(
        std::chrono::system_clock::now().time_since_epoch().count()));
    std::uniform_int_distribution<int> dist(0, 35);
    std::string RandomName;

    do
    {
        if ((!dir.empty()) && (dir.back() != UNIX_DIRECTORY_SEPARATOR) &&
            (dir.back() != WINDOWS_DIRECTORY_SEPARATOR))
        {
            RandomName = dir + DIRECTORY_SEPARATOR;
        }
        else
        {
            RandomName = dir;
        }
        int Char;

        for (size_t i = 0; i < 8u; i++)
        {
            Char = dist(engine);

            if (Char < 10)
            {
                RandomName += char('0' + Char);
            }
            else
            {
                RandomName += char('a' - 10 + Char);
            }
        }

        RandomName += suffix;
    } while (exist(RandomName));

    return RandomName;
}

#if 0
// -----------------------------------------------------------------------------
bool Path::move(const std::string& from, const std::string& to)
{
    if (!isFile(from))
        return false;

    std::string To = to;

    // Check whether To is a directory and append the
    // filename of from
    if (isDir(To))
        To += STRING_PREFERRED_DIRECTORY_SEPARATOR + fileName(from);

    if (isDir(To))
        return false;

#    if defined(_WIN32)

    // The target must not exist under WIN32 for rename to succeed.
    if (exist(To) && !remove(To))
        return false;

#    endif // WIN32

    bool success = (::rename(from.c_str(), To.c_str()) == 0);

    if (!success)
    {
        {
            std::ifstream in(from.c_str());
            std::ofstream out(To.c_str());

            out << in.rdbuf();

            success = out.good();
        }

        OS_REMOVE(from.c_str());
    }

    return success;
}

// -----------------------------------------------------------------------------
std::vector<std::string> Path::compilePattern(const std::string& pattern)
{
    std::string::size_type pos = 0;
    std::string::size_type start = 0;
    std::string::size_type end = 0;
    std::vector<std::string> PatternList;

    while (pos != std::string::npos)
    {
        start = pos;
        pos = pattern.find_first_of("*?", pos);

        end = std::min(pos, pattern.length());

        if (start != end)
        {
            PatternList.push_back(pattern.substr(start, end - start));
        }
        else
        {
            PatternList.push_back(pattern.substr(start, 1));
            pos++;
        }
    };

    return PatternList;
}

// -----------------------------------------------------------------------------
bool Path::match(const std::string& name,
                 const std::vector<std::string>& patternList)
{
    std::vector<std::string>::const_iterator it = patternList.begin();
    std::vector<std::string>::const_iterator end = patternList.end();
    std::string::size_type at = 0;
    std::string::size_type after = 0;

    bool Match = true;

    while (it != end && Match)
    {
        Match = matchInternal(name, *it++, at, after);
    }

    return Match;
}
#endif

// -----------------------------------------------------------------------------
bool Path::isRelativePath(const std::string& path)
{
    std::string Path = normalize(path);

    if (Path.length() == 0)
        return false;

    if ((Path[0] == UNIX_DIRECTORY_SEPARATOR ||
         Path[0] == WINDOWS_DIRECTORY_SEPARATOR))
        return false;

    // Windows path with drive letter
    if (Path.length() > 1 && Path[1] == ':')
        return false;

    return true;
}

#if 0
// -----------------------------------------------------------------------------
bool Path::makePathRelative(std::string& absolutePath, const std::string& relativeTo)
{
    if (isRelativePath(absolutePath) || isRelativePath(relativeTo))
        return false; // Nothing can be done.

    std::string RelativeTo = normalize(relativeTo);

    if (isFile(RelativeTo))
        RelativeTo = dirName(RelativeTo);

    if (!isDir(RelativeTo))
        return false;

    absolutePath = normalize(absolutePath);

    size_t i, imax = std::min(absolutePath.length(), RelativeTo.length());

    for (i = 0; i < imax; i++)
    {
        if (absolutePath[i] != RelativeTo[i])
            break;
    }

    // We need to retract to the beginning of the current directory.
    if (i != imax)
    {
        i = absolutePath.find_last_of('/', i) + 1;
    }

#    if defined(_WIN32)

    if (i == 0)
    {
        return false; // A different drive letter we cannot do anything
    }

#    endif // _WIN32

    RelativeTo = RelativeTo.substr(i);

    std::string relativePath;

    while (RelativeTo != "")
    {
        relativePath += "../";
        RelativeTo = dirName(RelativeTo);
    }

    if (relativePath != "")
    {
        absolutePath = relativePath + absolutePath.substr(i);
    }
    else
    {
        absolutePath = absolutePath.substr(i + 1);
    }

    return true;
}

// -----------------------------------------------------------------------------
bool Path::makePathAbsolute(std::string& relativePath, const std::string& absoluteTo)
{
    if (!isRelativePath(relativePath) || isRelativePath(absoluteTo))
        return false; // Nothing can be done.

    std::string AbsoluteTo = normalize(absoluteTo);

    if (isFile(AbsoluteTo))
        AbsoluteTo = dirName(AbsoluteTo);

    if (!isDir(AbsoluteTo))
        return false;

    relativePath = normalize(relativePath);

    while (!relativePath.compare(0, 3, "../"))
    {
        AbsoluteTo = dirName(AbsoluteTo);
        relativePath = relativePath.substr(3);
    }

    relativePath = AbsoluteTo + DIRECTORY_SEPARATOR + relativePath;

    return true;
}

// -----------------------------------------------------------------------------
bool Path::matchInternal(const std::string& name,
                         const std::string pattern,
                         std::string::size_type& at,
                         std::string::size_type& after)
{
    bool Match = true;

    switch (pattern[0])
    {
    case '*':
        if (at != std::string::npos)
        {
            after = at;
            at = std::string::npos;
        }
        break;

    case '?':
        if (at != std::string::npos)
        {
            ++at;
            Match = (name.length() >= at);
        }
        else
        {
            ++after;
            Match = (name.length() >= after);
        }
        break;

    default:
        if (at != std::string::npos)
        {
            Match = (name.compare(at, pattern.length(), pattern) == 0);
            at += pattern.length();
        }
        else
        {
            at = name.find(pattern, after);
            Match = (at != std::string::npos);
            at += pattern.length();
        }
        break;
    }

    return Match;
}
#endif

// -----------------------------------------------------------------------------
// TODO: should be replaced by canonicalPath ?
std::string Path::normalize(const std::string& path)
{
    if (path.empty())
        return {};

    // Detect the type of path (Windows or Unix)
    char preferred_separator = Path::preferredSeparator(path);

    // First, convert all separators to Unix format for normalization
    std::string clean_path = path;

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
std::string Path::canonicalPath(const std::string& path)
{
    if (path.empty())
        return {};

    // Determine the original separator style and work internally with Unix
    // separators
    char original_separator = Path::preferredSeparator(path);
    std::string current_path = toUnixSeparators(path);

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
            (path.length() >= 2 && path[0] == '.' &&
             (path[1] == '/' || path[1] == '\\'));
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
bool Path::isLargeFile(std::istream& input_stream)
{
    std::streampos pos = 0;
    input_stream.seekg(0, std::ios::end);
    pos = input_stream.tellg();
    input_stream.seekg(0);

    return pos >= 0xffffffff;
}

// -----------------------------------------------------------------------------
bool Path::hasTrailingSlash(const std::string& path)
{
    return (path.size() >= 1u) && (path.back() == WINDOWS_DIRECTORY_SEPARATOR ||
                                   path.back() == UNIX_DIRECTORY_SEPARATOR);
}

// -----------------------------------------------------------------------------
std::string Path::toZipArchiveSeparators(const std::string& path)
{
    return CONVERT_TO_PREFERRED_SEPARATORS(path);
}

// -----------------------------------------------------------------------------
bool Path::hasMixedSeparators(const std::string& path)
{
    bool hasWindowsSep = false;
    bool hasUnixSep = false;

    for (size_t i = 0; i < path.length(); ++i)
    {
        if (path[i] == WINDOWS_DIRECTORY_SEPARATOR)
            hasWindowsSep = true;
        else if (path[i] == UNIX_DIRECTORY_SEPARATOR)
            hasUnixSep = true;

        if (hasWindowsSep && hasUnixSep)
            return true;
    }

    return false;
}

// -----------------------------------------------------------------------------
std::string Path::toUnixSeparators(const std::string& path)
{
    std::string result = path;
    for (size_t i = 0; i < result.length(); ++i)
    {
        if (result[i] == WINDOWS_DIRECTORY_SEPARATOR)
            result[i] = UNIX_DIRECTORY_SEPARATOR;
    }
    return result;
}

// -----------------------------------------------------------------------------
std::string Path::toWindowsSeparators(const std::string& path)
{
    std::string result = path;
    for (size_t i = 0; i < result.length(); ++i)
    {
        if (result[i] == UNIX_DIRECTORY_SEPARATOR)
            result[i] = WINDOWS_DIRECTORY_SEPARATOR;
    }
    return result;
}

// -----------------------------------------------------------------------------
std::string Path::toNativeSeparators(const std::string& path)
{
#if defined(_WIN32)
    return toWindowsSeparators(path);
#else
    return toUnixSeparators(path);
#endif
}

// -----------------------------------------------------------------------------
size_t Path::getFileSize(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        return 0; // Return 0 if file doesn't exist or cannot be opened
    }
    std::streampos size = file.tellg();
    file.close();
    return (size >= 0) ? static_cast<size_t>(size) : 0;
}
