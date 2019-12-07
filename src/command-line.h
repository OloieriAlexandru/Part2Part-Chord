#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <iostream>
#include <string>
#include <vector>
#include <ostream>
#include <unordered_map>
#include <cstring>

namespace cmd{
    enum commandId{
        LISTALL,
        CLOSE,
        NOC // the type of all invalid commands
    };

    enum commandOptionType{
        STRING,
        NUMBER,
        BOOLEAN
    };

    class commandParser;

    class optionResult{
    public:
    };

    class commandResult{
    public:
        commandId                       id;
        std::vector<optionResult>       options;
                                        commandResult();
    };

    class commandOption;

    class commandOptionDefaultValue{
    private:
        commandOptionType               type;
        int                             numberValue;
        bool                            booleanValue;
        std::string                     stringValue;
    public: 
        void                            addValue(commandOptionType optionType, void* value);
        const void*                     getValue() const;

        friend std::ostream&            operator<<(std::ostream& out, const commandOption& cmdOption);
    };

    class commandOption{
    private:
        std::string                     name;
        commandOptionType               type;
        commandOptionDefaultValue       defaultValue;
    public:
                                        commandOption(std::string optionName);

        void                            addDefaultValue(commandOptionType optionType, void* defValue);
    
        friend std::ostream&            operator<<(std::ostream& out, const commandOption& cmdOption);
    };

    class commandInfo{
    private:
        commandId                       id;
        std::vector<cmd::commandOption> options;
        std::string                     name;
        std::string                     description;

        friend class                    commandParser;
    public:
                                        commandInfo(commandId commandId, const char* commandName, const char* commandDescription);

        void                            addStringOption(const char* optionName, const char* defaultValue);
        void                            addNumberOption(const char* optionName, int defaultValue);
        void                            addBooleanOption(const char* optionName, bool defaultValue);
    
        friend std::ostream&            operator<<(std::ostream& out, const commandInfo& cmdInfo);
    };

    class commandParser{
    private:
        std::vector<commandInfo>                commands;
        std::unordered_map<commandId, int>      commandIndex;
    public:
        commandResult                   parse(const std::string& str);

        void                            addCommand(commandId id, const char* commandName, const char* commandDescription);
        void                            addCommandOptionString(commandId id, const char* optionName, const char* defaultValue);
        void                            addCommandOptionNumber(commandId id, const char* optionName, int defaultValue);
        void                            addCommandOptionBoolean(commandId id, const char* optionName, bool defaultValue);

        friend std::ostream&            operator<<(std::ostream& out, const commandParser& parser);
    };
}

#endif // COMMAND_LINE_H