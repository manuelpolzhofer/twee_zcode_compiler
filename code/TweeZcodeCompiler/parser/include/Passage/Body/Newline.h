//
// Created by lars on 31.05.15.
//

#ifndef NEWLINE_H
#define NEWLINE_H

#include "BodyPart.h"

#include <string>

class Newline : public BodyPart {

public:
    virtual Newline* clone() const {
        return new Newline(*this);
    }

    Newline();

    std::string to_string();
};


#endif //NEWLINE_H
