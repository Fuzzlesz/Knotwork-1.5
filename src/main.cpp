#include <spdlog/sinks/basic_file_sink.h>

void SetupLog() {
    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");

    auto pluginName = Version::NAME;
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));

    spdlog::set_default_logger(std::move(loggerPtr));
#ifdef DEBUG
    spdlog::set_level(spdlog::level::debug);
    spdlog::flush_on(spdlog::level::debug);
#else
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);
#endif

    //Pattern
    spdlog::set_pattern("%v");
}

namespace QuestPatching {
    void ReadSetting() {

    }

    bool EditQuests() {
        try {
            auto* dataHandler = RE::TESDataHandler::GetSingleton();
            auto configs = clib_util::distribution::get_configs(R"(Data\SKSE\Plugins\Knotwork)");
            for (auto& config : configs) {
                _loggerInfo("Reading Config: {}", config);
                CSimpleIniA ini;
                ini.SetUnicode();
                ini.LoadFile(config.c_str());

                std::list<CSimpleIniA::Entry> modNames = std::list<CSimpleIniA::Entry>();
                ini.GetAllSections(modNames);

                for (auto& modName : modNames) {
                    auto* name = modName.pItem;
                    std::list<CSimpleIniA::Entry> entries = std::list<CSimpleIniA::Entry>();
                    ini.GetAllKeys(name, entries);

                    for (auto& id : entries) {
                        if (!clib_util::string::is_only_hex(id.pItem)) {
                            _loggerInfo(" ");
                            _loggerError("    >Failed to resolve {}:{}", name, id.pItem);
                            _loggerInfo(" ");
                            continue;
                        }

                        auto formID = clib_util::string::to_num<RE::FormID>(id.pItem, true);
                        auto* quest = dataHandler->LookupForm<RE::TESQuest>(formID, name);
                        if (!quest) {
                            _loggerInfo(" ");
                            _loggerError("    >Converted {}:{} to formID ({}), but doesn't point to a quest.", name, id.pItem, formID);
                            _loggerInfo(" ");
                            continue;
                        }

                        std::uint8_t raw = ini.GetLongValue(name, id.pItem);
                        quest->data.questType = static_cast<RE::QUEST_DATA::Type>(raw); // Type casting to resolve compile error
                        _loggerInfo("    >Setting {} to {}", quest->fullName, raw);
                    }
                    _loggerInfo(" ");
                }
                _loggerInfo("________________________________________");
            }
            _loggerInfo("Finished reading configs");
            _loggerInfo("________________________________________");
            _loggerInfo("NONE Quest Patching:");
            const auto& questArray = dataHandler->GetFormArray<RE::TESQuest>();

            for (auto* quest : questArray) {
                if (quest->data.questType == 0) {
                    quest->data.questType = 8;
                }
            }
            return true;
        }
        catch (std::exception& e) { // Catching exception by reference
            _loggerError("Failed to perform a necessary check. Error code: {}", e.what());
            return false;
        }
    }
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message) {
    switch (a_message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        if (!QuestPatching::EditQuests()) {
            _loggerError("Failed to patch all quests.");
        }
        else {
            _loggerInfo("Quests patched.");
        }
        break;
    default:
        break;
    }
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
    a_info->infoVersion = Version::MAJOR, Version::MINOR, Version::PATCH;
    a_info->name = "ContainerDistributionFramework";
    a_info->version = Version::MAJOR;

    if (a_skse->IsEditor()) {
        _loggerError("WRONG VERSION OF THE GAME");
        return false;
    }

    const auto ver = a_skse->RuntimeVersion();
    if (ver

        < SKSE::RUNTIME_1_5_39
        ) {
        _loggerError("WRONG VERSION OF THE GAME");
        return false;
    }

    return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse) {
    SetupLog();
    _loggerInfo("Starting up {}.", Version::NAME);
    _loggerInfo("Plugin Version: {}.", Version::VERSION);
    _loggerInfo("-------------------------------------------------------------------------------------");
    SKSE::Init(a_skse);

    auto messaging = SKSE::GetMessagingInterface();
    messaging->RegisterListener(MessageHandler);
    return true;
}