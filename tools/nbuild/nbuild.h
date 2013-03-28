#ifndef NBUILD_H
#define NBUILD_H

#include <vector>
#include <string>
#include <map>

#include "args.h"

using namespace std;

#define MACRO_DEFINES "-D"
#define SOURCE_FILE "-S"
#define EXCLUDE_FILE "-X"

#define NAME "--name"
#define CONFIGURATION "--config"
#define BINARY_TYPE "--lib-variant" // 'debug' or 'release'
#define PROJECT_DIR "--project"
#define OUTPUT_DIR "--dst"
#define BOOT_MODULE_LIST "--boot-modules"
#define MODULE_LIST "--modules"
#define EXCLUDE_FILES "--exclude-files"
#define VERBOSE "--verbose"
#define CLEAN "--clean"

string require(Arguments* args, string key);

/**
 * Returns the set of source files to compile, using SOURCE_FILE
 * and EXCLUDE_FILE preixes. Returns a list of files relative to
 * the PROJECT_DIR.
 */
vector<string> getSourceFiles(Arguments* args);
const char* mosyncdir();
void sh(const char* cmd, bool hideOutput = false, const char* shownCmdLine = 0);
void error(const char* msg, int errorCode = 1);

#endif
