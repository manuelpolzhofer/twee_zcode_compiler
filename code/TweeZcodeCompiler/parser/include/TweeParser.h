#ifndef TWEEPARSER_PARSER_H
#define TWEEPARSER_PARSER_H

/**
 * Example usage:
 * Twee::TweeParser parser;
 * parser.parse();
*/

#include <iostream>

namespace Twee {

    class TweeParser {
    public:
        TweeParser(std::istream* input);
        ~TweeParser();

        int parse();

    private:
        class TweeParserImpl;
        TweeParserImpl* impl;
    };
}

#endif
