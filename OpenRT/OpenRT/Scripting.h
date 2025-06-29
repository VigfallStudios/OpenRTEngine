#ifndef SCRIPT_H
#define SCRIPT_H

#include <iostream>
#include <vector>
#include <string>
#include <SDL3/SDL.h>
#include "Vector.h"

class ScriptCompiler
{
public:
	ScriptCompiler();
	void AddScript(std::string name);
	void Update(Vec3* cam, const bool* keys);

private:
	std::vector<std::string> scriptNames;
	bool started = false;

	void RunStartFunction(const std::string& scriptContent);
	void RunUpdateFunctions(Vec3* cam, const bool* keys);
};

#endif