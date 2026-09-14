// -----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
// -----------------------------------------------------------------------------

#include "utils/Path.hpp"
#include "utils/FilePath.hpp"
#include "utils/OS.hpp"

#include <chrono>
#include <filesystem>
#include <istream>
#include <random>
#include <sstream>
#include <system_error>
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
    std::error_code ec;
    const auto cwd = std::filesystem::current_path(ec);
    if (ec)
    {
        return {};
    }
    return toNativeSeparators(pathToUtf8(cwd));
}

// -----------------------------------------------------------------------------
bool Path::isFile(const std::string& path)
{
    if (path.empty())
    {
        return false;
    }
    std::error_code ec;
    return std::filesystem::is_regular_file(utf8ToPath(path), ec);
}

// -----------------------------------------------------------------------------
bool Path::isDir(const std::string& path)
{
    if (path.empty())
    {
        return false;
    }
    std::error_code ec;
    return std::filesystem::is_directory(utf8ToPath(path), ec);
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
    if (p_path.empty())
    {
        return false;
    }
    std::error_code ec;
    const auto st = std::filesystem::status(utf8ToPath(p_path), ec);
    if (ec)
    {
        return false;
    }
    return std::filesystem::is_regular_file(st) ||
           std::filesystem::is_directory(st);
}

// -----------------------------------------------------------------------------
bool Path::isReadable(const std::string& p_path)
{
    if (p_path.empty())
    {
        return false;
    }
    const auto native = utf8ToPath(p_path);
#if defined(_WIN32)
    return _waccess(native.c_str(), 0x4) == 0;
#else
    return ::access(native.c_str(), R_OK) == 0;
#endif
}

// -----------------------------------------------------------------------------
bool Path::isWritable(const std::string& p_path)
{
    if (p_path.empty())
    {
        return false;
    }
    const auto native = utf8ToPath(p_path);
#if defined(_WIN32)
    return _waccess(native.c_str(), 0x2) == 0;
#else
    return ::access(native.c_str(), W_OK) == 0;
#endif
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
    if (p_dir.empty() && p_parent.empty())
    {
        return false;
    }

    if (p_dir.empty())
    {
        return isDir(p_parent) && isWritable(p_parent);
    }

    const std::filesystem::path dest =
        p_parent.empty() ? utf8ToPath(p_dir)
                         : (utf8ToPath(p_parent) / utf8ToPath(p_dir));

    std::error_code ec;
    if (std::filesystem::is_directory(dest, ec))
    {
        return isWritable(pathToUtf8(dest));
    }

    const auto parent = dest.parent_path();
    if (!parent.empty() && parent != dest)
    {
        if (std::filesystem::exists(parent, ec) &&
            std::filesystem::is_directory(parent, ec) &&
            !isWritable(pathToUtf8(parent)))
        {
            return false;
        }
    }

    std::filesystem::create_directories(dest, ec);
    return !ec && std::filesystem::is_directory(dest, ec);
}

// -----------------------------------------------------------------------------
void Path::removeDir(const std::string& p_foldername)
{
    std::error_code ec;
    std::filesystem::remove_all(utf8ToPath(p_foldername), ec);
}

// -----------------------------------------------------------------------------
bool Path::remove(const std::string& p_path)
{
    if (isDir(p_path))
    {
        removeDir(p_path);
        return true;
    }

    if (isFile(p_path))
    {
        std::error_code ec;
        return std::filesystem::remove(utf8ToPath(p_path), ec);
    }

    return false;
}

// -----------------------------------------------------------------------------
std::vector<std::string> Path::filesFromDir(const std::string& p_path,
                                            const bool p_recurse)
{
    std::vector<std::string> files;
    const auto root = utf8ToPath(p_path);
    std::error_code ec;

    if (p_recurse)
    {
        auto it = std::filesystem::recursive_directory_iterator(
            root,
            std::filesystem::directory_options::skip_permission_denied,
            ec);
        if (ec)
        {
            return files;
        }
        const auto end = std::filesystem::recursive_directory_iterator();
        for (; it != end; it.increment(ec))
        {
            if (ec)
            {
                ec.clear();
                continue;
            }
            if (it->is_regular_file(ec) && !ec)
            {
                files.push_back(toNativeSeparators(pathToUtf8(it->path())));
            }
        }
        return files;
    }

    auto it = std::filesystem::directory_iterator(root, ec);
    if (ec)
    {
        return files;
    }
    const auto end = std::filesystem::directory_iterator();
    for (; it != end; it.increment(ec))
    {
        if (ec)
        {
            ec.clear();
            continue;
        }
        files.push_back(toNativeSeparators(pathToUtf8(it->path())));
    }
    return files;
}

// -----------------------------------------------------------------------------
std::string Path::getTempDirectory()
{
    std::error_code ec;
    auto temp_dir = std::filesystem::temp_directory_path(ec);
    if (ec)
    {
        temp_dir = utf8ToPath("/tmp");
    }

    std::string result = pathToUtf8(temp_dir);
    if ((!result.empty()) && (result.back() != UNIX_DIRECTORY_SEPARATOR) &&
        (result.back() != WINDOWS_DIRECTORY_SEPARATOR))
    {
        result += DIRECTORY_SEPARATOR;
    }
    return result;
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
    std::error_code ec;
    const auto size = std::filesystem::file_size(utf8ToPath(p_path), ec);
    return ec ? 0 : static_cast<size_t>(size);
}

// -----------------------------------------------------------------------------
std::string Path::canonicalPath(const std::string& p_destination_dir)
{
    std::string dest;

    if (p_destination_dir.empty())
    {
        dest = Path::normalize(currentPath());
    }
    else if (Path::root(p_destination_dir).empty())
    {
        dest = Path::normalize(currentPath() + DIRECTORY_SEPARATOR +
                               p_destination_dir);
    }
    else
    {
        dest = Path::normalize(p_destination_dir);
    }

    // Trailing slash is mandatory to avoid matching "john" and "johnny".
    // The if is important to avoid adding an extra trailing slash if
    // destination is a root path.
    if (!hasTrailingSlash(dest))
        dest += Path::preferredSeparator(dest);

    return dest;
}

// -----------------------------------------------------------------------------
bool Path::isZipSlip(const std::string& p_file_path,
                     const std::string& p_destination_dir)
{
    std::string dest = Path::canonicalPath(p_destination_dir);
    std::string file;
    if (Path::root(p_file_path).empty())
    {
        file = Path::canonicalPath(dest + p_file_path);
    }
    else
    {
        file = p_file_path;
    }
    return file.compare(0, dest.length(), dest) != 0;
}

// -----------------------------------------------------------------------------
Path::InvalidEntryReason
Path::checkControlCharacters(const std::string& p_entry_name)
{
    for (size_t i = 0; i < p_entry_name.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(p_entry_name[i]);

        // Check for control characters (ASCII codes 0-31) except for UTF-8
        // continuation bytes
        if (c < 32 && (i == 0 || (c & 0xC0) != 0x80))
        {
            return Path::InvalidEntryReason::CONTROL_CHARACTERS;
        }
    }

    return InvalidEntryReason::VALID_ENTRY;
}

// -----------------------------------------------------------------------------
Path::InvalidEntryReason Path::isValidEntry(std::string const& p_entry_name)
{
    if (p_entry_name.empty())
        return InvalidEntryReason::EMPTY_ENTRY;

    // Check for control characters
    auto result = Path::checkControlCharacters(p_entry_name);
    if (result != Path::InvalidEntryReason::VALID_ENTRY)
    {
        return result;
    }

    // Check for Zip Slip attack: reject an entry whose first path segment is a
    // parent-directory reference ("..", "../x", "..\\x"). A relative entry that
    // starts with ".." can only ever resolve outside the destination, so it is
    // rejected up-front. Interior references such as "foo/../Test1" are left to
    // Path::isZipSlip(), which resolves the final path and verifies it stays
    // inside the destination (this keeps legitimate cases working).
    //
    // We must NOT reject legitimate dot-files such as ".hidden", ".gitignore"
    // or "..foo" whose name merely begins with a dot: the previous
    // find_first_of("..") test wrongly flagged all of them.
    if (Path::startsWithParentDirectoryReference(p_entry_name))
    {
        return InvalidEntryReason::ZIP_SLIP;
    }

    // Absolute paths are not valid entry names
    if (!Path::root(p_entry_name).empty())
    {
        return InvalidEntryReason::ABSOLUTE_PATH;
    }

    return InvalidEntryReason::VALID_ENTRY;
}

// -----------------------------------------------------------------------------
bool Path::startsWithParentDirectoryReference(const std::string& p_entry_name)
{
    // The leading segment is a parent reference when the name is exactly ".."
    // or starts with ".." immediately followed by a directory separator.
    if ((p_entry_name.size() >= 2u) && (p_entry_name[0] == '.') &&
        (p_entry_name[1] == '.'))
    {
        if (p_entry_name.size() == 2u)
        {
            return true;
        }
        const char next = p_entry_name[2];
        if ((next == UNIX_DIRECTORY_SEPARATOR) ||
            (next == WINDOWS_DIRECTORY_SEPARATOR))
        {
            return true;
        }
    }
    return false;
}

std::string Path::getInvalidEntryReason(InvalidEntryReason p_reason)
{
    switch (p_reason)
    {
        case InvalidEntryReason::VALID_ENTRY:
            return "Valid entry";
        case InvalidEntryReason::EMPTY_ENTRY:
            return "cannot be empty";
        case InvalidEntryReason::FORBIDDEN_CHARACTERS:
            return "contains forbidden characters";
        case InvalidEntryReason::CONTROL_CHARACTERS:
            return "contains control characters";
        case InvalidEntryReason::ABSOLUTE_PATH:
            return "is an absolute path";
        case InvalidEntryReason::ZIP_SLIP:
            return "could be used to escape the destination directory";
        default:
            return "Unknown reason";
    }
}