#include "rpg_text.h"

namespace rpg
{

namespace
{
    Language current_language = default_language;
}

Language language()
{
    return current_language;
}

void set_language(Language language)
{
    current_language = language;
}

bn::string_view localized(const bn::string_view& text)
{
    for(int index = 0, size = text.size(); index < size; ++index)
    {
        if(text[index] == language_separator)
        {
            return current_language == Language::english ? text.substr(0, index) : text.substr(index + 1);
        }
    }

    return text;
}

}
