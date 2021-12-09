#include "Distribute.h"
#include "LookupConfigs.h"
#include "LookupForms.h"

namespace MessageHandler
{
	struct detail
	{
		static void send_event()
		{
			SKSE::ModCallbackEvent modEvent{
				"KID_KeywordDistributionDone",
				RE::BSFixedString(),
				0.0f,
				nullptr
			};

			auto modCallback = SKSE::GetModCallbackEventSource();
			modCallback->SendEvent(&modEvent);
		}
	};

	void Distribute(SKSE::MessagingInterface::Message* a_message)
	{
		if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
			if (Lookup::Forms::GetForms()) {
				Distribute::AddKeywords();
			}
			detail::send_event();
		}
	}

	void NoDistribute(SKSE::MessagingInterface::Message* a_message)
	{
		if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
			detail::send_event();
		}
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S:%e] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::NAME.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	logger::info("loaded plugin");

	SKSE::Init(a_skse);

	auto messaging = SKSE::GetMessagingInterface();
	if (Lookup::Config::Read()) {
		messaging->RegisterListener(MessageHandler::Distribute);
	} else {
		messaging->RegisterListener(MessageHandler::NoDistribute);
	}

	return true;
}
