#include "glob.hpp"

// clang-format off
std::regex globToRegex(const std::string& glob)
{
    std::string regex_pattern;
    regex_pattern.reserve(glob.size() * 2);

    regex_pattern += "^"; // Start of line

    for (size_t i = 0; i < glob.size(); ++i)
    {
        char c = glob[i];
        switch (c)
        {
            case '*':
                regex_pattern += ".*"; // * correspond to any number of characters
                break;
            case '?':
                regex_pattern += "."; // ? corresponds to a single character
                break;
            case '.': case '+': case '(': case ')': case '{': case '}':
            case '|': case '^': case '$': case '[': case ']': case '\\':
                // Escape special characters in regex
                regex_pattern += '\\';
                regex_pattern += c;
                break;
            default:
                regex_pattern += c;
                break;
        }
    }

    regex_pattern += "$"; // End of line
    return std::regex(regex_pattern);
}
// clang-format on
