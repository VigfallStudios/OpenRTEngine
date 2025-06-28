#ifndef SCRIPT_H
#define SCRIPT_H

#include <iostream>
#include <vector>
#include <string>

class ScriptCompiler
{
public:
	ScriptCompiler();
	void AddScript(std::string name);
	void Update();

private:
	std::vector<std::string> scriptNames;
	bool started = false;

	void RunStartFunction(const std::string& scriptContent);
	void RunUpdateFunctions();
};

#endif