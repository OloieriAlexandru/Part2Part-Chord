#include "command-line.h"

// --------- optionResult ----------------------------------------------------------------------------------//


// --------- commandResult ----------------------------------------------------------------------------------//


// --------- commandOptionDefaultValue ----------------------------------------------------------------------------------//

void cmd::commandOptionDefaultValue::addValue(commandOptionType optionType, void* value){
    type = optionType;
    switch(optionType){
        case STRING:
            stringValue = *((std::string*)value);
            break;
        case NUMBER:
            numberValue = *((int*)value);
            break;
        case BOOLEAN:
            booleanValue = *((bool*)value);
            break;
        default:
            break;
    }
    return;
}

void* cmd::commandOptionDefaultValue::getValue(){
    switch(type){
        case STRING:
            return &stringValue;
        case NUMBER:
            return &numberValue;
        case BOOLEAN:
            return &booleanValue;
        default:
            break;
    }
    return NULL;
}

// --------- commandOption ----------------------------------------------------------------------------------//


// --------- commandInfo ----------------------------------------------------------------------------------//

cmd::commandInfo::commandInfo(commandId id, const char* commandDescription){
    std::cout<<"hmm\n";
}

// --------- commandParser ----------------------------------------------------------------------------------//

cmd::commandResult cmd::commandParser::parse(const std::string& str){
    cmd::commandResult result;

    return result;
}