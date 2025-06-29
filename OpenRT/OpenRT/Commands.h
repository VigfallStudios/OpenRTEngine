#ifndef CMD_H
#define CMD_H

#include <iostream>
#include <sstream>
#include <fstream>

#include "Scripting.h"

class CommandManager
{
public:
	CommandManager(ScriptCompiler &c);
	void RunString(std::string cmd);

private:
	ScriptCompiler& compiler;
};

#endif