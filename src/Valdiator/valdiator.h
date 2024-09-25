#pragma once

#include <sqlite3.h>
#include <libpq-fe.h>
#include "Debug/Logger.h"

class Validator
{
public:

	Validator(sqlite3** db,AppLogger* NewLogger, PGconn** Connection);


private:

	AppLogger* Logger;

};
