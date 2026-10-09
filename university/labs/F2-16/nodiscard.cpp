enum class ParseError { ok, empty, not_a_number };

[[nodiscard]] ParseError parse_portions(const char* text, int& out)
{
    if (text[0] == '\0') {
        return ParseError::empty;
    }
    out = text[0] - '0';
    return ParseError::ok;
}

int main()
{
    int portions = 0;
    parse_portions("", portions);   // the error code is ignored
    return portions;
}
