//
// Created by lars on 30.06.15.
//

#ifndef UNARYOPERATION_H
#define UNARYOPERATION_H

#include "Expression.h"

#include <string>
#include <memory>

class UnaryOperation : public Expression {

private:
    UnOps op;
    std::unique_ptr<Expression> expression;

public:

    UnaryOperation(UnOps, const Expression *);

    UnaryOperation(UnOps, const Expression &);

    UnaryOperation(const UnaryOperation &unaryOperation) {
        this->op = unaryOperation.getOperator();
        this->expression.reset(unaryOperation.getExpression()->clone());
    }

    UnOps getOperator() const;

    std::string getOperatorString() const;

    const std::unique_ptr<Expression> &getExpression() const;

    std::string to_string() const;

    virtual UnaryOperation *clone() const {
        return new UnaryOperation(*this);
    }
};


#endif //UNARYOPERATION_H
