
#include <iostream>
#include "Debug/Logger.h"
#include "Bot/BotHandler.h"
#include "Utils/Utils.h"
#include "tgbot/tgbot.h"
#include <sqlite3.h>
#include <libpq-fe.h>
#include "Valdiator/valdiator.h"


AppLogger* Logger = new AppLogger();
Validator* ValidatorPtr = nullptr;

using namespace TgBot;
using namespace std;

sqlite3* pDB = nullptr;
PGconn* AnalyticDatabase = nullptr;

#define APPLICATIONLOG "Application"


int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);
	Logger->RegisterLogOutput();
	ValidatorPtr = new Validator(&pDB, Logger, &AnalyticDatabase);

	BotHandler* Handler = new BotHandler(Logger,pDB,AnalyticDatabase);
	Handler->Launch();
	Logger->UnregisterLogOutput();



	std::cout << GetExecutableOsPath() << std::endl;
	PQfinish(AnalyticDatabase);
    return 0;
}
