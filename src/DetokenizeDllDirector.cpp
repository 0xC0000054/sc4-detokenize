/*
 * This file is part of SC4Detokenize, a DLL Plugin for SimCity 4
 * that adds a cheat to display text token values.
 *
 * Copyright (C) 2025 Nicholas Hayes
 *
 * SC4Detokenize is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * SC4Detokenize is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SC4Detokenize.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "Logger.h"
#include "version.h"
#include "cIGZApp.h"
#include "cIGZCheatCodeManager.h"
#include "cIGZCommandParameterSet.h"
#include "cIGZCommandServer.h"
#include "cIGZCOM.h"
#include "cIGZFrameWork.h"
#include "cIGZMessage2Standard.h"
#include "cIGZMessageServer2.h"
#include "cISC4App.h"
#include "cISC4City.h"
#include "cISCStringDetokenizer.h"
#include "cRZAutoRefCount.h"
#include "cRZBaseString.h"
#include "cRZMessage2COMDirector.h"
#include "CommandParameterSetUtil.h"
#include "GZServPtrs.h"
#include "SC4NotificationDialog.h"
#include "StringViewUtil.h"

#include <array>
#include <string>
#include <vector>

#include <Windows.h>
#include "wil/resource.h"
#include "wil/result.h"
#include "wil/win32_helpers.h"

static constexpr uint32_t kSC4MessagePostCityInit = 0x26D31EC1;
static constexpr uint32_t kSC4MessagePreCityShutdown = 0x26D31EC2;
static constexpr uint32_t kMessageCheatIssued = 0x230E27AC;

static constexpr std::array<uint32_t, 2> RequiredNotifications =
{
	kSC4MessagePostCityInit,
	kSC4MessagePreCityShutdown,
};

static constexpr uint32_t kDetokenizeDllDirector = 0x45519D62;

static constexpr uint32_t kDetokenizeCheatID = 0x73D84360;
static constexpr std::string_view kDetokenizeCheatString = "Detokenize";

static constexpr std::string_view PluginLogFileName = "SC4Detokenize.log";

namespace
{
	std::filesystem::path GetDllFolderPath()
	{
		wil::unique_cotaskmem_string modulePath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

		std::filesystem::path temp(modulePath.get());

		return temp.parent_path();
	}

	bool TryProcessCommand(const std::string_view& input, cRZBaseString& output)
	{
		bool result = false;

		const std::string_view command = StringViewUtil::Trim(input, "#");

		if (command.size() > 0)
		{
			cIGZCommandServerPtr commandServer;

			if (commandServer)
			{
				uint32_t commandID = 0;
				cIGZCommandParameterSet* pInputParams = nullptr;

				if (commandServer->ConvertStringToCommand(
					command.data(),
					command.size(),
					commandID,
					pInputParams))
				{
					cRZAutoRefCount<cIGZCommandParameterSet> outputParams;

					if (commandServer->CreateCommandParameterSet(outputParams.AsPPObj()))
					{
						commandServer->ExecuteCommand(commandID, pInputParams, outputParams);

						CommandParameterSetUtil::OutputToString(commandServer, outputParams, output);
					}

					if (pInputParams)
					{
						pInputParams->Release();
					}

					result = true;
				}
			}
		}

		return result;
	}
}

class DetokenizeDllDirector final : public cRZMessage2COMDirector
{
public:
	DetokenizeDllDirector()
		: pCheatMgr(nullptr),
		  pDetokenizer(nullptr)
	{
		std::filesystem::path dllFolderPath = GetDllFolderPath();

		std::filesystem::path logFilePath = dllFolderPath;
		logFilePath /= PluginLogFileName;

		Logger& logger = Logger::GetInstance();
		logger.Init(logFilePath, LogLevel::Error);
		logger.WriteLogFileHeader("SC4Detokenize v" PLUGIN_VERSION_STR);
	}

	uint32_t GetDirectorID() const
	{
		return kDetokenizeDllDirector;
	}

	bool OnStart(cIGZCOM* pCOM)
	{
		mpFrameWork->AddHook(this);
		return true;
	}

private:
	bool DoMessage(cIGZMessage2* pMsg)
	{
		switch (pMsg->GetType())
		{
		case kSC4MessagePostCityInit:
			PostCityInit();
			break;
		case kSC4MessagePreCityShutdown:
			PreCityShutdown();
			break;
		case kMessageCheatIssued:
			ProcessCheat(static_cast<cIGZMessage2Standard*>(pMsg));
			break;
		}

		return true;
	}

	void PostCityInit()
	{
		pCheatMgr->AddNotification2(this, 0);
		pCheatMgr->RegisterCheatCode(
			kDetokenizeCheatID,
			cRZBaseString(kDetokenizeCheatString.data(), kDetokenizeCheatString.size()));
	}

	void PreCityShutdown()
	{
		pCheatMgr->RemoveNotification2(this, 0);
		pCheatMgr->UnregisterCheatCode(kDetokenizeCheatID);
	}

	void ProcessCheat(cIGZMessage2Standard* pStandardMsg)
	{
		const uint32_t cheatID = static_cast<uint32_t>(pStandardMsg->GetData1());

		if (cheatID == kDetokenizeCheatID)
		{
			const cIGZString* cheatStr = static_cast<const cIGZString*>(pStandardMsg->GetVoid2());

			if (cheatStr)
			{
				const std::string_view cheatStrAsStringView(cheatStr->Data(), cheatStr->Strlen());
				std::string_view command;

				if (cheatStrAsStringView.size() > kDetokenizeCheatString.size())
				{
					command = StringViewUtil::Trim(cheatStrAsStringView.substr(kDetokenizeCheatString.size()), " ");
				}

				if (command.size() > 0)
				{
					cRZBaseString output;

					if (!TryProcessCommand(command, output))
					{
						cRZBaseString tokenizedValue;

						if (command[0] != '#')
						{
							tokenizedValue.Append("#", 1);
						}

						tokenizedValue.Append(command.data(), command.size());

						if (command[command.size() - 1] != '#')
						{
							tokenizedValue.Append("#", 1);
						}

						pDetokenizer->Detokenize(tokenizedValue, output);
					}

					SC4NotificationDialog::ShowDialog(
						output,
						cRZBaseString(kDetokenizeCheatString.data(), kDetokenizeCheatString.size()));
				}
			}
		}
	}

	bool PostAppInit()
	{
		Logger& logger = Logger::GetInstance();

		cIGZApp* const pApp = mpFrameWork->Application();

		if (pApp)
		{
			cRZAutoRefCount<cISC4App> sc4App;

			if (pApp->QueryInterface(GZIID_cISC4App, sc4App.AsPPVoid()))
			{
				pCheatMgr = sc4App->GetCheatCodeManager();
				pDetokenizer = sc4App->GetStringDetokenizer();
			}
		}

		if (pCheatMgr && pDetokenizer)
		{
			cIGZMessageServer2Ptr ms2;

			for (uint32_t messageID : RequiredNotifications)
			{
				if (!ms2->AddNotification(this, messageID))
				{
					logger.WriteLine(LogLevel::Error, "Failed to subscribe to the required notifications.");
					return false;
				}
			}
		}
		else
		{
			logger.WriteLine(LogLevel::Error, "Failed to get the cheat manager and/or string detokenizer.");
			return false;
		}

		return true;
	}

	cIGZCheatCodeManager* pCheatMgr;
	cISCStringDetokenizer* pDetokenizer;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static DetokenizeDllDirector sDirector;
	return &sDirector;
}