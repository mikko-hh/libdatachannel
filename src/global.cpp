/**
 * Copyright (c) 2020-2021 Paul-Louis Ageneau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Converters/UTF8Converter.h"
#include "plog/Formatters/FuncMessageFormatter.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include "plog/Log.h"
#include "plog/Logger.h"
//
#include "global.hpp"

#include "impl/init.hpp"

#include <mutex>

namespace rtc {
	struct LogAppender : public plog::IAppender {
		synchronized_callback<LogLevel, string> callback;

		void write(const plog::Record& record) override {
			const auto severity = record.getSeverity();
			auto formatted = plog::FuncMessageFormatter::format(record);
			formatted.pop_back(); // remove newline

			const auto& converted =
				plog::UTF8Converter::convert(formatted); // does nothing on non-Windows systems

			if (!callback(static_cast<LogLevel>(severity), converted))
				std::cout << plog::severityToString(severity) << " " << converted << std::endl;
		}
	};
}

namespace {
using Logger = plog::Logger<PLOG_DEFAULT_INSTANCE_ID>;
Logger* g_logger = nullptr;
rtc::LogAppender* g_appender = nullptr;

void plogInit(plog::Severity severity, plog::IAppender *appender) {
	if (!g_logger) {
		PLOG_DEBUG << "Initializing logger";
		g_logger = new Logger(severity);
		if (appender) {
			g_logger->addAppender(appender);
		} else {
			using ConsoleAppender = plog::ColorConsoleAppender<plog::TxtFormatter>;
			static ConsoleAppender *consoleAppender = new ConsoleAppender();
			g_logger->addAppender(consoleAppender);
		}
	} else {
		g_logger->setMaxSeverity(severity);
		if (appender)
			g_logger->addAppender(appender);
	}
}
void plogUninit() {
	delete g_logger;
	g_logger = nullptr;
	delete g_appender;
	g_appender = nullptr;
}
} // namespace

namespace rtc {

void InitLogger(LogLevel level, LogCallback callback) {
	const auto severity = static_cast<plog::Severity>(level);
	static std::mutex mutex;
	std::lock_guard lock(mutex);
	if (g_appender) {
		g_appender->callback = std::move(callback);
		plogInit(severity, nullptr); // change the severity
	} else if (callback) {
		g_appender = new LogAppender();
		g_appender->callback = std::move(callback);
		plogInit(severity, g_appender);
	} else {
		plogInit(severity, nullptr); // log to cout
	}
}

void InitLogger(plog::Severity severity, plog::IAppender *appender) {
	plogInit(severity, appender);
}
void UninitLogger() {
	plogUninit();
}

void Preload() { impl::Init::Instance().preload(); }
std::shared_future<void> Cleanup() { return impl::Init::Instance().cleanup(); }

void SetSctpSettings(SctpSettings s) { impl::Init::Instance().setSctpSettings(std::move(s)); }

RTC_CPP_EXPORT std::ostream &operator<<(std::ostream &out, LogLevel level) {
	switch (level) {
	case LogLevel::Fatal:
		out << "fatal";
		break;
	case LogLevel::Error:
		out << "error";
		break;
	case LogLevel::Warning:
		out << "warning";
		break;
	case LogLevel::Info:
		out << "info";
		break;
	case LogLevel::Debug:
		out << "debug";
		break;
	case LogLevel::Verbose:
		out << "verbose";
		break;
	default:
		out << "none";
		break;
	}
	return out;
}

} // namespace rtc
