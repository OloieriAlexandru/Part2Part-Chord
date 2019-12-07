#include "command-line.h"

// --------- optionResult ----------------------------------------------------------------------------------//


// --------- commandResult ----------------------------------------------------------------------------------//

cmd::commandResult::commandResult(){
    id = cmd::commandId::NOC;
}

// --------- commandOptionDefaultValue ----------------------------------------------------------------------------------//

void cmd::commandOptionDefaultValue::addValue(commandOptionType optionType, void* value){
    type = optionType;
    switch(optionType){
        case STRING:
            stringValue = std::string((const char*)value);
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

const void* cmd::commandOptionDefaultValue::getValue() const{
    switch(type){
        case STRING:
            return (const std::string*)&stringValue;
        case NUMBER:
            return (const int*)&numberValue;
        case BOOLEAN:
            return (const bool*)&booleanValue;
        default:
            break;
    }
    return NULL;
}

// --------- commandOption ----------------------------------------------------------------------------------//

cmd::commandOption::commandOption(std::string optionName){
    name = optionName;
}

void cmd::commandOption::addDefaultValue(cmd::commandOptionType optionType, void* defValue){
    type = optionType;
    defaultValue.addValue(optionType, defValue);
}

namespace cmd{
    std::ostream& operator<<(std::ostream& out, const cmd::commandOption& cmdOption){
        out<<"\toption: "<<cmdOption.name<<"    default: ";
        switch(cmdOption.type){
            case STRING:
                out<<*((const std::string*)cmdOption.defaultValue.getValue());
                break;
            case NUMBER:
                out<<*((const int*)cmdOption.defaultValue.getValue());
                break;
            case BOOLEAN:
                out<<(*((const bool*)cmdOption.defaultValue.getValue())?"true" : "false");
                break;
            default:
                break;
        }
        return out;
    }
}

// --------- commandInfo ----------------------------------------------------------------------------------//

cmd::commandInfo::commandInfo(commandId commandId, const char* commandName, const char* commandDescription){
    id = commandId;
    name = std::string(commandName);
    description = std::string(commandDescription);
}

void cmd::commandInfo::addStringOption(const char* optionName, const char* defaultValue){
    std::string name(optionName);
    cmd::commandOption cmdOption(name);

    cmd::commandOptionType type = cmd::commandOptionType::STRING;

    cmdOption.addDefaultValue(type,(void*)(defaultValue));
    options.push_back(cmdOption);
}

void cmd::commandInfo::addNumberOption(const char* optionName, int defaultValue){
    std::string name(optionName);
    cmd::commandOption cmdOption(name);

    cmd::commandOptionType type = cmd::commandOptionType::NUMBER;

    cmdOption.addDefaultValue(type,(void*)(&defaultValue));
    options.push_back(cmdOption);
}

void cmd::commandInfo::addBooleanOption(const char* optionName, bool defaultValue){
    std::string name(optionName);
    cmd::commandOption cmdOption(name);

    cmd::commandOptionType type = cmd::commandOptionType::BOOLEAN;

    cmdOption.addDefaultValue(type,(void*)(&defaultValue));
    options.push_back(cmdOption);
}

namespace cmd{
    std::ostream& operator<<(std::ostream& out, const cmd::commandInfo& cmdInfo){
        out<<cmdInfo.name<<"\n\t--> "<<cmdInfo.description<<'\n';
        for (auto& option:cmdInfo.options){
            out<<option<<'\n';
        }
        return out;
    }
}

// --------- commandParser ----------------------------------------------------------------------------------//

cmd::commandResult cmd::commandParser::parse(const std::string& str){
    cmd::commandResult result;

    char* stringCmd = strdup(str.c_str());
    if (!stringCmd){
        return result; // error ?
    }

    char* arguments = strtok(stringCmd, " \t\n");
    std::string enteredCommand(arguments);

    int cmdIndex = -1;
    for (int i=0;i<commands.size();++i){
        if (enteredCommand == commands[i].name){
            cmdIndex = i;
            result.id = commands[i].id;
            break;
        }
    }

    if (cmdIndex == -1){
        return result; // invalid command
    }
    
    arguments = strtok(NULL, " \t\n");
    

    if (stringCmd){
        delete[] stringCmd;
    }
    return result;
}

void cmd::commandParser::addCommand(commandId id, const char* commandName, const char* commandDescription){
    commandIndex[id] = commands.size();
    commands.push_back(cmd::commandInfo(id, commandName, commandDescription));
}

void cmd::commandParser::addCommandOptionString(commandId id, const char* optionName, const char* defaultValue){
    if (!commandIndex.count(id)){
        return;
    }
    int index = commandIndex[id];
    commands[index].addStringOption(optionName, defaultValue);
}

void cmd::commandParser::addCommandOptionNumber(commandId id, const char* optionName, int defaultValue){
    if (!commandIndex.count(id)){
        return;
    }
    int index = commandIndex[id];
    commands[index].addNumberOption(optionName, defaultValue);
}

void cmd::commandParser::addCommandOptionBoolean(commandId id, const char* optionName, bool defaultValue){
    if (!commandIndex.count(id)){
        return;
    }
    int index = commandIndex[id];
    commands[index].addBooleanOption(optionName, defaultValue);
}

namespace cmd{
    std::ostream& operator<<(std::ostream& out, const cmd::commandParser& parser){
        int no = 1;
        for (auto& command : parser.commands){
            out<<(no++)<<". "<<command;
        }
        return out;
    }
}