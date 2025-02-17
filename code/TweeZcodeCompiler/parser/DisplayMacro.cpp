//
// Created by lars on 26.06.15.
//

#include "include/Passage/Body/Macros/DisplayMacro.h"

DisplayMacro::DisplayMacro(const std::string passage) {
    this->passage = passage;
}

const std::string &DisplayMacro::getPassage() const {
    return this->passage;
}

std::string DisplayMacro::to_string() const {
    return "DisplayMacro: " + this->getPassage();
}

void DisplayMacro::accept(IBodyPartsVisitor& visitor) const {
    visitor.visit(*this);
}