// -----------------------------------------------------------------------------
// Copyright (c) 2022 Quentin Quadrat <lecrapouille@gmail.com>
// https://github.com/Lecrapouille/zipper distributed under MIT License.
// Based on https://github.com/sebastiandev/zipper/tree/v2.x.y distributed under
// MIT License. Copyright (c) 2015 -- 2022 Sebastian <devsebas@gmail.com>
//
// Copyright (C) 2010 - 2014 by Pedro Mendes, Virginia Tech Intellectual
// Properties, Inc., University of Heidelberg, and The University
// of Manchester.
// All rights reserved.
//
// Copyright (C) 2008 - 2009 by Pedro Mendes, Virginia Tech Intellectual
// Properties, Inc., EML Research, gGmbH, University of Heidelberg,
// and The University of Manchester.
// All rights reserved.
//
// Copyright (C) 2005 - 2007 by Pedro Mendes, Virginia Tech Intellectual
// Properties, Inc. and EML Research, gGmbH.
// All rights reserved.
// -----------------------------------------------------------------------------

#include "utils/OS.hpp"
#include "utils/Path.hpp"

#include <fstream>
#include <iterator>
#include <sstream>
#include <numeric>

using namespace zipper;

const std::string Path::Separator(DIRECTORY_SEPARATOR);

// -----------------------------------------------------------------------------
std::string Path::currentPath()
{
    char buffer[1024u];
    return (OS_GETCWD(buffer, sizeof(buffer)) ? std::string(buffer) : std::string(""));
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
bool Path::exist(const std::string& path)
{
    STAT st;

    if (stat(path.c_str(), &st) == -1)
        return false;

#if defined(_WIN32)
    return ((st.st_mode & S_IFREG) == S_IFREG || (st.st_mode & S_IFDIR) == S_IFDIR);
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
    std::string::size_type start = path.find_last_of(Separator);

#if defined(_WIN32) // WIN32 also understands '/' as the separator.
    if (start == std::string::npos)
    {
        start = path.find_last_of(DIRECTORY_SEPARATOR);
    }
#endif

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
#ifdef _WIN32
    // / on Windows is root on current drive
    if (path.length() == 1 && path[0] == '/')
        return true;

    // X:/ is root including drive letter
    return path.length() == 3 && path[1] == ':';
#else
    return path.length() == 1 && path[0] == '/';
#endif
}

// -----------------------------------------------------------------------------
std::string Path::root(const std::string& path)
{
#ifdef _WIN32
    // Check if we have an absolute path with drive,
    // otherwise return the root for the current drive.
    if (path[1] == ':') // Colon is on Windows only allowed here to denote a
                        // preceeding drive letter => absolute path
        return path.substr(0, 3);
    else
        return DIRECTORY_SEPARATOR;
#else
    (void) path;
    return DIRECTORY_SEPARATOR;
#endif
}

// -----------------------------------------------------------------------------
std::string Path::dirName(const std::string& path)
{
    if (path == ".")
        return "";

    if (path == "..")
        return "";

    if (path == "//")
        return "//";

    if (Path::isRoot(path))
        return path;

    size_t pos = 0;
    if ((pos = path.rfind(DIRECTORY_SEPARATOR)) != std::string::npos)
    {
        // Single = intended
        if (pos == 0) // /usr
            return root(path);

#ifdef _WIN32
        if ((pos == 1) && path[1] == ':') // X:/foo
            return root(path);
#endif
        // regular/path or /regular/path
        return path.substr(0, pos);
    }

    // single relative directory
    return "";
}

// -----------------------------------------------------------------------------
// Note: WIN32 also understands '/' as the separator.
std::string Path::suffix(const std::string& path)
{
    std::string::size_type start = path.find_last_of(Separator);

#if defined(_WIN32)
    if (start == std::string::npos)
    {
        start = path.find_last_of(DIRECTORY_SEPARATOR);
    }
#endif

    if (start == std::string::npos)
    {
        start = 0;
    }
    else
    {
        start++; // We do not want the separator.
    }

    std::string::size_type end = path.find_last_of(".");

    if (end == std::string::npos || end < start)
        return {};

    return path.substr(end);
}

// -----------------------------------------------------------------------------
bool Path::createDir(const std::string& dir, const std::string& parent)
{
    std::string Dir;

    if (!parent.empty())
    {
        Dir = parent + Separator;
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

// -----------------------------------------------------------------------------
bool Path::removeFiles(const std::string& pattern,
                       const std::string& path)
{
    bool success = true;
    std::vector<std::string> PatternList;

    PatternList = compilePattern(pattern);

#if defined(_WIN32)

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
                if (OS_REMOVE((path + Separator + Utf8).c_str()) != 0)
                    success = false;
            }
            else
            {
                if (OS_RMDIR((path + Separator + Utf8).c_str()) != 0)
                    success = false;
            }
        }
    } while (_findnext(hList, &Entry) == 0);

    _findclose(hList);

#else //! _WIN32

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
                if (OS_RMDIR((path + Separator + Utf8).c_str()) != 0)
                    success = false;
            }
            else
            {
                if (OS_REMOVE((path + Separator + Utf8).c_str()) != 0)
                    success = false;
            }
        }
    }

    closedir(pDir);

#endif // _WIN32

    return success;
}

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
    // Pour Windows, utiliser l'API FindFirst/FindNext
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
            if (Path::isDir(path + Path::Separator + filename))
            {
                std::vector<std::string> moreFiles =
                        Path::filesFromDir(path + Path::Separator + filename, recurse);
                std::copy(moreFiles.begin(), moreFiles.end(),
                          std::back_inserter(files));
                continue;
            }
        }
        files.push_back(path + Path::Separator + filename);
    } while (_findnext(hList, &Entry) == 0);

    _findclose(hList);
#else
    // Code existant pour les systèmes POSIX
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
            if (Path::isDir(path + Path::Separator + filename))
            {
                std::vector<std::string> moreFiles =
                        Path::filesFromDir(path + Path::Separator + filename, recurse);
                std::copy(moreFiles.begin(), moreFiles.end(),
                          std::back_inserter(files));
                continue;
            }
        }
        files.push_back(path + Path::Separator + filename);
    }

    closedir(dir);
#endif

    return files;
}

// -----------------------------------------------------------------------------
std::string Path::createTmpName(const std::string& dir, const std::string& suffix)
{
    std::string RandomName;

    do
    {
        RandomName = dir + Separator;
        int Char;

        for (size_t i = 0; i < 8u; i++)
        {
            Char = static_cast<int>((rand() / double(RAND_MAX)) * 35.0);

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

// -----------------------------------------------------------------------------
bool Path::move(const std::string& from, const std::string& to)
{
    if (!isFile(from))
        return false;

    std::string To = to;

    // Check whether To is a directory and append the
    // filename of from
    if (isDir(To))
        To += Separator + fileName(from);

    if (isDir(To))
        return false;

#if defined(_WIN32)

    // The target must not exist under WIN32 for rename to succeed.
    if (exist(To) && !remove(To))
        return false;

#endif // WIN32

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

// -----------------------------------------------------------------------------
bool Path::isRelativePath(const std::string& path)
{
#if defined(_WIN32)

    std::string Path = normalize(path);

    if (Path.length() < 2)
        return true;

    if (Path[1] == ':')
        return false;

    if (Path[0] == '/' && Path[1] == '/')
        return false;

    return true;

#else //! _WIN32

    return (path.length() < 1 || path[0] != '/');

#endif // _WIN32
}

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

#if defined(_WIN32)

    if (i == 0)
    {
        return false; // A different drive letter we cannot do anything
    }

#endif // _WIN32

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

// -----------------------------------------------------------------------------
std::string Path::normalize(const std::string& path)
{
    std::string clean_path = path;

#if defined(_WIN32)
    // converts all '\' to '/' (only on WIN32)
    size_t i, imax;

    for (i = 0, imax = clean_path.length(); i < imax; i++)
    {
        if (clean_path[i] == '\\')
            clean_path[i] = '/';
    }

#endif

    // Remove leading './'
    while (!clean_path.compare(0, 2, "./"))
    {
        clean_path = clean_path.substr(2);
    }

    // Collapse '//' to '/'
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
    if (clean_path.back() == DIRECTORY_SEPARATOR_CHAR)
        clean_path.pop_back();
    return clean_path;
}

// -----------------------------------------------------------------------------
std::string Path::canonicalPath(const std::string& path)
{
    if (path.empty())
        return {};

    // If the path starts with a / we must preserve it
    bool starts_with_slash = (path[0] == DIRECTORY_SEPARATOR_CHAR);

    // If the path does not end with a / we need to remove the
    // extra / added by the join process
    //bool ends_with_slash = (path.back() == DIRECTORY_SEPARATOR_CHAR);

    // Store each element of the path
    std::vector<std::string> segments;

    size_t current;
    size_t next = size_t(-1);

    do {
        // Extract the element of the path
        current = size_t(next + 1u);
        next = path.find_first_of("/\\", current);
        std::string segment(path.substr(current, next - current));
        size_t size = segment.length();

        // Skip empty string: keep initial (i.e. ///)
        if (size == 0u)
        {
            continue;
        }

        // skip "." string: keep initial
        else if ((segment == ".") && (segments.size() > 0u))
        {
            continue;
        }

        // Remove "..": replace "foo/../bar" by "bar"
        else if (segment == "..")
        {
            // Ignore if .. follows initial '/'
            if (starts_with_slash && (segments.size() == 0u))
            {
                continue;
            }

            // Segments [ ..., "foo", "bar" ] becomes [ ..., "foo" ]
            if ((segments.size() > 0u) && (segments.back() != ".."))
            {
                segments.pop_back();
                continue;
            }
        }

        segments.push_back(segment);
    } while (next != std::string::npos);

    // Manage the case where the path starts with '/'
    std::string clean_path;
    if (starts_with_slash)
    {
        if (segments.empty() || (segments[0] != "."))
        {
            clean_path += DIRECTORY_SEPARATOR;
        }
    }

    // Join the vector as a single string, every element is separated by the
    // folder separator.
    for (auto const& it: segments)
    {
        clean_path += it;
        clean_path += DIRECTORY_SEPARATOR;
    }

    // Manage the case "./" where the '/' was adding during the join.
    if ((clean_path.size() == 2u) && (clean_path[0] == '.') &&
        (clean_path[1] == DIRECTORY_SEPARATOR_CHAR))
    {
        return Path::Separator;
    }

    // Remove the last '/'
    if (/*!ends_with_slash &&*/ (clean_path.length() > 1u))
    {
        clean_path.pop_back();
    }

    return clean_path;
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
    return (path.size() >= 1u) && (path.back() == '\\' || path.back() == '/');
}
