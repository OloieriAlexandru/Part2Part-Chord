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
        ADD_FILE,
        SEARCH_FILE,
        DOWNLOAD_FILE,
        LIST_FILES_TO_DOWNLOAD,
        LIST_FILES,
        CONFIG_ADD_FILE,
        CONFIG_REMOVE_FILE,
        CONFIG_REMOVE_ALL,
        CONFIG_LIST_FILES,
        CONFIG_AUTO_ADD,
        LISTALL,
        CLOSE,
        EMPTYLINE, // returned when the entered line is empty
        WOC, // the type of all invalid commands
        WOCOPT // the type for commands with invalid options
    };

    enum commandOptionType{
        STRING,
        NUMBER,
        BOOLEAN
    };

    std::string commandEnumToString(commandId cmdId);
    std::string commandOptionTypeEnumToString(commandOptionType cmdOptionType);

    class commandOptionDefaultValue;
    class commandOption;
    class commandInfo;
    class commandParser;

    class optionResult{
    public:
        commandOptionType               type;
        int                             numberValue;
        bool                            booleanValue;
        std::string                     stringValue;
        std::string                     name;

                                        optionResult() {}
                                        optionResult(const std::string optionName, const commandOptionDefaultValue& defaultValue);

        void                            updateValue(void* newValue);

        friend std::ostream&            operator<<(std::ostream& out, const optionResult& optResult);
    };

    class commandResult{
    public:
        commandId                       id;
        std::vector<optionResult>       options;
                                        commandResult();
        void                            initOptions(const commandInfo& cmdInfo);
        void                            updateOption(const std::string& optionName, void* newValue);
        void                            clearOptions();

        std::string                     getStringOptionValue(const std::string& optionName) const;
        int                             getNumberOptionValue(const std::string& optionName) const;
        bool                            getBooleanOptionValue(const std::string& optionName) const;

        friend std::ostream&            operator<<(std::ostream& out, const commandResult& cmdResult);
    };

    class commandOptionDefaultValue{
    private:
        commandOptionType               type;
        int                             numberValue;
        bool                            booleanValue;
        std::string                     stringValue;

        friend class                    optionResult;
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

        friend class                    commandResult;
        friend class                    commandParser;
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

        friend class                    commandResult;
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

        bool                                    parseAndCheckOptionValue(char arguments[], int startPos, int lastPos, const cmd::commandOption& opt, int position, cmd::commandResult& result);
        bool                                    parseOption(char arguments[], int firstPos, int lastPos, int cmdIndex, cmd::commandResult& result);
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