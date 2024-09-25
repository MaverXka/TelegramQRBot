#pragma once


#include "tgbot/tgbot.h"
#include <memory>
#include "QRCodeGenerator/qrcodegen.hpp"
#include <sqlite3.h>
#include <libpq-fe.h>
#include <string>

class AppLogger;

using namespace qrcodegen;

class BotHandler
{
public:
	BotHandler(AppLogger* NewLogger, sqlite3* NewDatabase, PGconn* NewAnalyticDB);

	void Launch();

private:

	AppLogger* Logger;
	sqlite3* Database;
	PGconn* AnalyticDB;
	sqlite3_stmt* STMT_CheckQRCode;
	sqlite3_stmt* STMT_AddQRCode;

	void PopulateCommandList();

	void ExecuteAnalyticQuery(const std::string& query);
	std::string BuildAnalyticAddRowQuery(std::string tableName, std::string message);
	
	void OnReciveAnyMessage(TgBot::Message::Ptr message);

	void saveQRCodeAsPNG(const QrCode& qr, const std::string& filename);

	std::unique_ptr<TgBot::Bot> tgBot;
	std::vector<TgBot::BotCommand::Ptr> BotCommandList;
	std::unordered_map<int64_t, bool> AwaitingLink;
	
	/// <returns>Filename if exists</returns>
	std::string isQRCodeExists(const std::string URL, bool& IsFound);



	/// <summary>
	/// Should be without .extension suffix
	/// </summary>
	/// <param name="URL"></param>
	/// <param name="Filename"></param>
	void AddQRCode(const std::string URL, const std::string Filename);

};
