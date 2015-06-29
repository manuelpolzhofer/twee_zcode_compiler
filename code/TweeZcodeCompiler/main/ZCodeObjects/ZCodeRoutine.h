//
// Created by philip on 27.06.15.
//

#ifndef PROJECT_ZCODEROUTINE_H
#define PROJECT_ZCODEROUTINE_H


#include "ZCodeContainer.h"
#include "ZCodeInstruction.h"
#include <map>

class ZCodeRoutine: public ZCodeContainer {
private:
    static std::map<std::string, std::shared_ptr<ZCodeRoutine>> routines;
    uint8_t localVariables;
public:
    void print(std::vector<std::bitset<8>> & code);
    ZCodeRoutine(uint8_t locVariables){
        containerOffset = 1;
       this->localVariables = locVariables;
    }

    static std::shared_ptr<ZCodeRoutine> getOrCreateRoutine(std::string name, uint8_t locVariables);
};


#endif //PROJECT_ZCODEROUTINE_H
