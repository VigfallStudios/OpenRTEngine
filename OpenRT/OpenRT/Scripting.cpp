#include "Scripting.h"
#include "Debug.h"

#include <fstream>
#include <sstream>

ScriptCompiler::ScriptCompiler()
{
	Debug.Log("RTScript Compiler instantiated");
}

void ScriptCompiler::AddScript(std::string name)
{
	Debug.Log("Script added " + name);
	scriptNames.push_back(name);
}

void ScriptCompiler::RunStartFunction(const std::string& scriptContent)
{
    // Super simple parser: looks for "fun Start()" and runs the block
    size_t startPos = scriptContent.find("fun Start()");
    if (startPos == std::string::npos) return;

    size_t braceOpen = scriptContent.find("{", startPos);
    size_t braceClose = scriptContent.find("}", braceOpen);
    if (braceOpen == std::string::npos || braceClose == std::string::npos) return;

    std::string body = scriptContent.substr(braceOpen + 1, braceClose - braceOpen - 1);

    // Parse body (naive): if "Debug.Log(...)" exists, extract message
    size_t logPos = body.find("Debug.Log");
    if (logPos != std::string::npos)
    {
        size_t quote1 = body.find("\"", logPos);
        size_t quote2 = body.find("\"", quote1 + 1);
        if (quote1 != std::string::npos && quote2 != std::string::npos)
        {
            std::string msg = body.substr(quote1 + 1, quote2 - quote1 - 1);
            Debug.Log("[Script] " + msg);
        }
    }
}

void ScriptCompiler::RunUpdateFunctions()
{
    for (const auto& scriptName : scriptNames)
    {
        std::ifstream file(scriptName);
        if (!file.is_open()) {
            Debug.LogError("Failed to open script: " + scriptName);
            continue;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string content = buffer.str();

        size_t startPos = content.find("fun Update()");
        if (startPos == std::string::npos) continue;

        size_t braceOpen = content.find("{", startPos);
        size_t braceClose = content.find("}", braceOpen);
        if (braceOpen == std::string::npos || braceClose == std::string::npos) continue;

        std::string body = content.substr(braceOpen + 1, braceClose - braceOpen - 1);

        // Naive run: check for Debug.Log
        size_t logPos = body.find("Debug.Log");
        if (logPos != std::string::npos) {
            size_t quote1 = body.find("\"", logPos);
            size_t quote2 = body.find("\"", quote1 + 1);
            if (quote1 != std::string::npos && quote2 != std::string::npos) {
                std::string msg = body.substr(quote1 + 1, quote2 - quote1 - 1);
                Debug.Log("[Script-Update] " + msg);
            }
        }
    }
}

void ScriptCompiler::Update()
{
    if (!started) {
        started = true;

        for (const auto& scriptName : scriptNames)
        {
            std::ifstream file(scriptName);
            if (!file.is_open()) {
                Debug.LogError("Failed to open script: " + scriptName);
                continue;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();

            RunStartFunction(content);
        }
    }

    RunUpdateFunctions();
}