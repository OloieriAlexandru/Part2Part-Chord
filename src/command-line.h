#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <iostream>
#include <string>
#include <vector>
#include <ostream>

namespace cmd{
    enum commandId{
        LISTALL,
        NOC // the type of all invalid commands
    };

    enum commandOptionType{
        STRING,
        NUMBER,
        BOOLEAN
    };

    class optionResult{
    private:

    public:
    };

    class commandResult{
    private:
        commandId                   id;
        std::vector<optionResult>   options;
    public:
    };

    class commandOptionDefaultValue{
    private:
        commandOptionType   type;
        int                 numberValue;
        bool                booleanValue;
        std::string         stringValue;
    public: 
        void                addValue(commandOptionType optionType, void* value);
        void*               getValue();
    };

    class commandOption{
    private:
        std::string                 name;
        commandOptionType           type;
        commandOptionDefaultValue   defaultValue;
    public:
                                    commandOption(std::string name);

        void                        addDefaultValue(commandOptionType optionType, void* defaultValue);
    };

    class commandInfo{
    private:
        std::vector<commandOption>      options;
        std::string                     description;
    public:
                                        commandInfo(commandId id, const char* commandDescription);

        void                            addStringOption(const char* optionName, const char* defaultValue);
        void                            addNumberOption(const char* optionName, int defaultValue);
        void                            addBooleanOption(const char* optionName, bool defaultValue);
    };

    class commandParser{
    private:
        std::vector<commandInfo>        commands;
    public:
        commandResult                   parse(const std::string& str);

        void                            addCommand(commandId id, const char* commandDescription);
        void                            addCommandOptionString(commandId id, const char* optionName, const char* defaultValue);
        void                            addCommandOptionNumber(commandId id, const char* optionName, int defaultValue);
        void                            addCommandOptionBoolean(commandId id, const char* optionName, bool defaultValue);

        friend std::ostream&            operator<<(std::ostream& out, const commandParser& parser);
    };
}

#endif // COMMAND_LINE_H