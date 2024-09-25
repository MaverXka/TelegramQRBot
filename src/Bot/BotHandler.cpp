#include "BotHandler.h"
#include "Utils/Utils.h"
#include "Config.h"
#include "Debug/Logger.h"
#include <boost/json.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <exception>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Utils/stb_image.h"
#include "Utils/stb_image_write.h"

#include <csignal>
#include <exception>

using namespace TgBot;
using namespace std;

#define BOTHANDLERCATEGORY "BotHandler"

void saveQRCodeAsPNG(const QrCode& qr, const std::string& filename);
string GetFinalQRCodeFilename(string Filename);

BotHandler::BotHandler(AppLogger* NewLogger, sqlite3* NewDatabase, PGconn* NewAnalyticDB) : Logger(NewLogger), Database(NewDatabase), AnalyticDB(NewAnalyticDB)
{

    const char* checkqrcode_request_str = "SELECT * FROM qrcodes WHERE link = ? LIMIT 1";
    int lresult;
    lresult = sqlite3_prepare_v2(Database, checkqrcode_request_str, -1, &STMT_CheckQRCode, NULL);
    if (lresult != SQLITE_OK)
    {
        string err = "Error to prepare check qr code sql request: " + string(sqlite3_errmsg(Database));
        Logger->LogMsg(LogVerb::FatalError, BOTHANDLERCATEGORY, err.c_str());
        throw std::runtime_error(err.c_str());;
    }

    const char* addqrcode_request_str = "INSERT INTO qrcodes (link, filename) VALUES (?, ?)";
    lresult = sqlite3_prepare_v2(Database, addqrcode_request_str, -1, &STMT_AddQRCode, NULL);
    if (lresult != SQLITE_OK)
    {
        string err = "Error to prepare add qr code sql request: " + string(sqlite3_errmsg(Database));
        Logger->LogMsg(LogVerb::FatalError, BOTHANDLERCATEGORY, err.c_str());
        throw std::runtime_error(err.c_str());;
    }


}

void BotHandler::Launch()
{
    Config& cfg = Config::Get();

    lQrCodeBorderSize = cfg.GetQRCodeBorderSize();

    tgBot = std::make_unique<TgBot::Bot>(cfg.GetBotToken());

    tgBot->getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {
        OnReciveAnyMessage(message);
    });

    signal(SIGINT, [](int s) {
        printf("SIGINT got\n");
        exit(0);
        });

    try {
        Logger->LogMsg(LogVerb::Log, BOTHANDLERCATEGORY, "Bot username: " + tgBot->getApi().getMe()->username);
        tgBot->getApi().deleteWebhook();

        TgLongPoll longPoll(*tgBot);
        while (true) {
            longPoll.start();
        }
    }
    catch (exception& e) {
        Logger->LogMsg(LogVerb::FatalError, BOTHANDLERCATEGORY, e.what());
    }

}

void BotHandler::PopulateCommandList()
{
}

void BotHandler::ExecuteAnalyticQuery(const std::string table, const std::string message)
{
    string query = R"(
    SELECT tg_bot_update($1, $2);   
)";

    const char* paramValues[2];
    paramValues[0] = message.c_str();
    paramValues[1] = table.c_str();

    PGresult* res = PQexecParams(AnalyticDB, query.c_str(), 2, NULL, paramValues, NULL, NULL, 0);
    //if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    //    Logger->LogMsg(LogVerb::FatalError, BOTHANDLERCATEGORY, string("Failed to execute analytic query: ") + PQerrorMessage(AnalyticDB));
        //PQclear(res);
    //}
    PQclear(res);
}

void BotHandler::OnReciveAnyMessage(TgBot::Message::Ptr message)
{
    if (StringTools::startsWith(message->text, "/start")) {
        tgBot->getApi().sendMessage(message->chat->id, "Hello, enter your link.");
        return;
    }

        std::string URL = message->text;
        if (URL.empty() || (URL.find("http://") != 0 && URL.find("https://") != 0 && URL.find("www.") != 0))
        {
            tgBot->getApi().sendMessage(message->chat->id, "There is not a link");
            return;
        }
        if (URL.length() >= 2048)
        {
            tgBot->getApi().sendMessage(message->chat->id, "Link contains to much characters");
            return;
        }

        bool bFound = false;
        std::string qrfilename = isQRCodeExists(message->text, bFound);
        if (bFound)
        {

            tgBot->getApi().sendMessage(message->chat->id, "There is your QR Code:");
            std::cout << "txt: " << GetFinalQRCodeFilename(qrfilename) << std::endl;
            tgBot->getApi().sendPhoto(message->chat->id, InputFile::fromFile(GetFinalQRCodeFilename(qrfilename), "image/png"));
            string outMsg = "Sending exist QR Code with URL: " + URL + " Filename: " + qrfilename;
            Logger->LogMsg(LogVerb::Log, BOTHANDLERCATEGORY, outMsg);
        }
        else
        {
            const QrCode qr = QrCode::encodeText(message->text.c_str(), QrCode::Ecc::LOW);
            boost::uuids::random_generator generator;
            boost::uuids::uuid qrcodeuuid = generator();
            string qrcodeid = boost::uuids::to_string(qrcodeuuid);
            saveQRCodeAsPNG(qr, qrcodeid, lQrCodeBorderSize);
            AddQRCode(message->text.c_str(), qrcodeid);

            tgBot->getApi().sendMessage(message->chat->id, "There is your QR Code:");
            tgBot->getApi().sendPhoto(message->chat->id, InputFile::fromFile(GetFinalQRCodeFilename(qrcodeid), "image/png"));
        }

        std::string table_name = Config::Get().GetAnalyticData().analyticTableName;
        ExecuteAnalyticQuery(table_name, message->text);


    
}

string GetFinalQRCodeFilename(string Filename)
{
    std::filesystem::path base_path = GetExecutableOsPath();
    base_path /= "generated";
    base_path /= Filename + ".png";
    return base_path.string();
}

void BotHandler::saveQRCodeAsPNG(const QrCode& qr, const std::string& filename, int qrCodeBorderSize) {
    int size = qr.getSize();
    int scale = 10;
    int dim = (size + 2 * qrCodeBorderSize) * scale;

    std::vector<uint8_t> image(dim * dim, 0xFF);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qr.getModule(x, y)) {
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        image[((y + qrCodeBorderSize) * scale + dy) * dim + (x + qrCodeBorderSize) * scale + dx] = 0x00;
                    }
                }
            }
        }
    }
    int writeresult = stbi_write_png(GetFinalQRCodeFilename(filename).c_str(), dim, dim, 1, image.data(), dim);
    if (!writeresult)
    {
        Logger->LogMsg(LogVerb::Error, BOTHANDLERCATEGORY, "Failed to write QR Code image file.");
        Logger->LogMsg(LogVerb::Error, BOTHANDLERCATEGORY, GetFinalQRCodeFilename(filename));
    }
}

std::string BotHandler::isQRCodeExists(const std::string URL, bool& IsFound)
{
    std::string res = "";
    sqlite3_bind_text(STMT_CheckQRCode, 1, URL.c_str(), -1, SQLITE_STATIC);
    int r;
    if ((r = sqlite3_step(STMT_CheckQRCode)) == SQLITE_ROW)
    {
        const char* flname = reinterpret_cast<const char*>(sqlite3_column_text(STMT_CheckQRCode, 1));
        IsFound = true;
        res = std::string(flname);
    }
    else
    {
        IsFound = false;
        res = "";
    }
    sqlite3_reset(STMT_CheckQRCode);
    return res;
}


void BotHandler::AddQRCode(const std::string URL, const std::string Filename)
{
    sqlite3_bind_text(STMT_AddQRCode, 1, URL.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(STMT_AddQRCode, 2, Filename.c_str(), -1, SQLITE_STATIC);
    if (sqlite3_step(STMT_AddQRCode) == SQLITE_DONE)
    {
        string outMsg = "Added new sql row with URL: " + URL + " Filename: " + Filename;
        Logger->LogMsg(LogVerb::Log, BOTHANDLERCATEGORY, outMsg);
        sqlite3_reset(STMT_AddQRCode);
    }
    else
    {
        string msg = string("Failed to add new QR Code to Database: ") + sqlite3_errmsg(Database);
        Logger->LogMsg(LogVerb::FatalError, BOTHANDLERCATEGORY, msg);
        sqlite3_reset(STMT_AddQRCode);

    }
}
