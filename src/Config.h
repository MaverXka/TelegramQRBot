#include <string>
#include "Utils/Utils.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <boost/json.hpp>
#include <sstream>

struct AnalyticDBData
{
	std::string analyticHostServer;
	std::string analyticHostPort;
	std::string analyticUser;
	std::string analyticDBName;
	std::string analyticPassword;
	std::string analyticTableName;
};

class Config
{

public:

	static Config& Get()
	{
		static Config instance;
		return instance;
	}
private:

	Config()
	{
		    std::cout << "throw2" << std::endl;
		std::ifstream ConfigFile(GetExecutableOsPath()+"/config.json");
		if (!ConfigFile.is_open())
		{
			std::cout << "Error file not found: config.json." << std::endl;
			throw std::runtime_error("Error, file not found: config.json.");
		}
		std::stringstream buffer;
		buffer << ConfigFile.rdbuf();

		nlohmann::json data = nlohmann::json::parse(buffer);
		try
		{
			botToken = data["telegramBotToken"];
			analyticDBData.analyticHostServer = data["analyticHost"];
			analyticDBData.analyticHostPort = data["analyticPort"];
			analyticDBData.analyticUser = data["analyticUser"];
			analyticDBData.analyticPassword = data["analyticPassword"];
			analyticDBData.analyticDBName = data["analyticDBname"];
			analyticDBData.analyticTableName = data["analyticTableName"];
			qrCodeBorderSize = data["qrCodeBorderSize"];
		}
		catch(nlohmann::json::type_error& ex)
		{
			std::cout << "Failed to init config: " << ex.what() << std::endl;
		}

	}

	~Config() = default;

	std::string botToken;
	int qrCodeBorderSize = 4;
	AnalyticDBData analyticDBData;


public:
	
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;

	std::string GetBotToken() { return botToken; };
	AnalyticDBData GetAnalyticData() { return analyticDBData; };
	int GetQRCodeBorderSize() { return qrCodeBorderSize; };



};
