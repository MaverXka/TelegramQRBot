#include "../Valdiator/valdiator.h"
#include "Utils/Utils.h"
#include <string>
#include "Config.h"
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

#define VALIDATORLOG "Validator"

Validator::Validator(sqlite3** db, AppLogger* NewLogger, PGconn** Connection) : Logger(NewLogger)
{

	fs::path dir = std::string(GetExecutableOsPath() + "/generated");
	if (!fs::exists(dir))
	{
		if (fs::create_directory(dir))
		{
			Logger->LogMsg(LogVerb::Log, VALIDATORLOG, "Generated folder is created");
		}
		else
		{
			Logger->LogMsg(LogVerb::FatalError, VALIDATORLOG, "Failed to create generated folder.");
		}
	}
	else
	{
		Logger->LogMsg(LogVerb::Log, VALIDATORLOG, "Generated folder is exists.");
	}

	int ExitCode = sqlite3_open(std::string(GetExecutableOsPath() + "/qrcodedatabase.db").c_str(), db);
	if (ExitCode) {
		Logger->LogMsg(LogVerb::FatalError, VALIDATORLOG, string("Failed to open QR Code database: ").append(sqlite3_errmsg(*db)));
		sqlite3_close(*db);
	}
	else
	{
		Logger->LogMsg(LogVerb::Log, VALIDATORLOG, "QR Code database opened successfully.");
	}

	const char* tablecreationsql = "CREATE TABLE IF NOT EXISTS qrcodes ("
		"link TEXT,"
		"filename TEXT);";

	char* errmsg;
	int rc = sqlite3_exec(*db, tablecreationsql, 0, 0, &errmsg);

	if (rc != SQLITE_OK)
	{
		Logger->LogMsg(LogVerb::FatalError, VALIDATORLOG, errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(*db);
	}

	Config& cfg = Config::Get();
	const char* connectionKeys[] = {"host", "port", "dbname", "user", "password", nullptr};
	const char* connectionValues[6];
	AnalyticDBData dbdata = cfg.GetAnalyticData();
	connectionValues[0] = dbdata.analyticHostServer.c_str();
	connectionValues[1] = dbdata.analyticHostPort.c_str();
	connectionValues[2] = dbdata.analyticDBName.c_str();
	connectionValues[3] = dbdata.analyticUser.c_str();
	connectionValues[4] = dbdata.analyticPassword.c_str();
	connectionValues[5] = nullptr;

	Logger->LogMsg(LogVerb::Log, VALIDATORLOG, string("Analytic database server: ") + cfg.GetAnalyticData().analyticHostServer);
	Logger->LogMsg(LogVerb::Log, VALIDATORLOG, string("Analytic database port: ") + cfg.GetAnalyticData().analyticHostPort.c_str());
	Logger->LogMsg(LogVerb::Log, VALIDATORLOG, string("Analytic database name: ") + cfg.GetAnalyticData().analyticDBName.c_str());
	Logger->LogMsg(LogVerb::Log, VALIDATORLOG, string("Analytic database user: ") + cfg.GetAnalyticData().analyticUser.c_str());
	PGconn* analyticConnection = PQconnectdbParams(connectionKeys, connectionValues,0);
	if (PQstatus(analyticConnection) != CONNECTION_OK) {
		Logger->LogMsg(LogVerb::FatalError, VALIDATORLOG, string("Connection failed to analytic database: ") + string(PQerrorMessage(analyticConnection)));
		PQfinish(analyticConnection);
		throw std::runtime_error("Connection failed to analytic database");
	}
	else
	{
		Logger->LogMsg(LogVerb::Log, VALIDATORLOG, "Connected to analytic database");
	}

	std::string query = "CREATE TABLE IF NOT EXISTS " + dbdata.analyticTableName + " ("
		"date DATE NOT NULL, "
		"urls TEXT[] NOT NULL"
		");";

	PGresult* res = PQexec(analyticConnection, query.c_str());
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		Logger->LogMsg(LogVerb::FatalError, VALIDATORLOG, string("Failed to execute validation analytic query: ") + string(PQerrorMessage(analyticConnection)));
		//std::cerr << PQerrorMessage(analyticConnection) << std::endl;
		PQclear(res);
	}
	string table_name = dbdata.analyticTableName;
	string dfuncquery = R"(
CREATE OR REPLACE FUNCTION tg_bot_update(p_url TEXT, p_table_name TEXT) RETURNS VOID AS $$
DECLARE
    record_exists BOOLEAN;
    url_exists BOOLEAN;
BEGIN
    EXECUTE format('SELECT EXISTS (SELECT 1 FROM %I WHERE date = CURRENT_DATE)', p_table_name) INTO record_exists;

    IF record_exists THEN
        EXECUTE format('SELECT array_position(urls, $1) IS NOT NULL FROM %I WHERE date = CURRENT_DATE', p_table_name) INTO url_exists USING p_url;

        IF NOT url_exists THEN
            EXECUTE format('UPDATE %I SET urls = array_append(urls, $1) WHERE date = CURRENT_DATE', p_table_name) USING p_url;
        END IF;
    ELSE
        EXECUTE format('INSERT INTO %I (date, urls) VALUES (CURRENT_DATE, ARRAY[$1])', p_table_name) USING p_url;
    END IF;
END;
$$ LANGUAGE plpgsql;
)";

	PGresult* res2 = PQexec(analyticConnection, dfuncquery.c_str());
	if (PQresultStatus(res2) != PGRES_COMMAND_OK) {
		Logger->LogMsg(LogVerb::FatalError, VALIDATORLOG, string("Failed to execute validation analytic add func query: ") + string(PQerrorMessage(analyticConnection)));
		//std::cerr << PQerrorMessage(analyticConnection) << std::endl;
		PQclear(res2);
	}

	*Connection = analyticConnection;

}
