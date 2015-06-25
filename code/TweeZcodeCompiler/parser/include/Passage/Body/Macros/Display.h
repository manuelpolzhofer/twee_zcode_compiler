//
// Created by lars on 24.06.15.
//

#ifndef DISPLAY_H
#define DISPLAY_H

#include "Macro.h"
#include "../Expressions/Expression.h"

#include <string>
#include <memory>

class Display : public Marco {


public:

    Display(std::unique_ptr<Expression> &);

    const std::unique_ptr<Expression> &getExpression();

    std::string to_string();

    virtual Display *clone() const {
        return new Display(*this);
    }

};

#endif //DISPLAY_H
