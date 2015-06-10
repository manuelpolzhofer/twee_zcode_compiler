//
// Created by Danni on 09.06.15.
//

#ifndef PROJECT_TWEECOMPILER_H
#define PROJECT_TWEECOMPILER_H

#include <TweeDoc.h>
#include <ostream>
#include <map>

class TweeCompiler {
public:
    void compile(TweeDoc& tweeDoc, std::ostream& out);
private:
    std::map<std::string, int> passageName2id;
};


#endif //PROJECT_TWEECOMPILER_H
