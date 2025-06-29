#include "Commands.h"
#include "Debug.h"

CommandManager::CommandManager(ScriptCompiler& c) : compiler(c) {}

void CommandManager::RunString(std::string cmd)
{
    if (cmd.rfind("/addscript", 0) == 0)
    {
        std::istringstream iss(cmd);
        std::string command, scriptName;
        iss >> command >> scriptName;

        if (!scriptName.empty())
        {
            compiler.AddScript(scriptName);
        }
        else
        {
            Debug.LogError("No script name provided.");
        }
    }
    else if (cmd.rfind("/createscript", 0) == 0)
    {
        std::istringstream iss(cmd);
        std::string command, fileName;
        iss >> command >> fileName;

        if (!fileName.empty())
        {
            std::ofstream file(fileName);
            if (file)
            {
                file << "// This function runs only at the start of the game.\n";
                file << "fun Start()\n";
                file << "{\n";
                file << "\n";
                file << "}\n\n";
                file << "// This function runs every frame.\n";
                file << "fun Update()\n";
                file << "{\n";
                file << "\n";
                file << "}\n";

                file.close();
                Debug.Log("Script created: " + fileName);
            }
            else
            {
                Debug.LogError("Failed to create script file: " + fileName);
            }
        }
        else
        {
            Debug.LogError("No filename provided.");
        }
    }
    else
    {
        Debug.LogWarning("Unknown command: " + cmd);
    }
}