#include "remoteapi.h"

#include <fstream>
#include <glob.h>
#include <iostream>
#include "logger.h"
#include <unistd.h>

#include "configcontainer.h"
#include "utils.h"

namespace newsboat {

RemoteApi::RemoteApi(ConfigContainer& c)
	: cfg(c)
{
}

bool RemoteApi::mark_articles_read(const std::vector<std::string>& guids)
{
	bool success = true;
	for (const auto& guid : guids) {
		if (!this->mark_article_read(guid, true)) {
			success = false;
		}
	}
	return success;
}

const std::string RemoteApi::read_password(const Filepath& file)
{
	glob_t exp;
	std::ifstream ifs;
	std::string pass = "";

	int res = glob(file.to_locale_string().c_str(), GLOB_ERR, nullptr, &exp);
	if (!res && exp.gl_pathc == 1 && exp.gl_pathv) {
		ifs.open(exp.gl_pathv[0]);
	}
	globfree(&exp);
	if (ifs.is_open()) {
		std::getline(ifs, pass);
	}

	return pass;
}

const std::string RemoteApi::eval_password(const std::string& cmd)
{
	LOG(Level::DEBUG, "RemoteApi::eval_password: running `%s`", cmd);
	std::string pass = utils::get_command_output(cmd);
	LOG(Level::DEBUG, "RemoteApi::eval_password: command printed out `%s'", pass);
	const auto pos = pass.find_first_of("\n\r");

	if (pos != std::string::npos) {
		pass.resize(pos);
		LOG(Level::DEBUG, "RemoteApi::eval_password: ...clipping that to `%s'", pass);
	}

	return pass;
}

Credentials RemoteApi::get_credentials(const std::string& scope,
	const std::string& name)
{
	std::string user = cfg.get_configvalue(scope + "-login");
	std::string pass = cfg.get_configvalue(scope + "-password");
	std::string pass_file = cfg.get_configvalue(scope + "-passwordfile");
	std::string pass_eval = cfg.get_configvalue(scope + "-passwordeval");
	std::string token = cfg.get_configvalue(scope + "-token");
	std::string token_file = cfg.get_configvalue(scope + "-tokenfile");
	std::string token_eval = cfg.get_configvalue(scope + "-tokeneval");

	if (!token.empty()) {
		return {"", "", token};
	}

	if (!token_file.empty()) {
		token = read_password(token_file);
		return {"", "", token};
	}

	if (!token_eval.empty()) {
		token = eval_password(token_eval);
		return {"", "", token};
	}

	bool flushed = false;

	if (user.empty()) {
		std::cout << std::endl;
		std::cout.flush();
		flushed = true;
		printf("Username for %s: ", name.c_str());
		std::cin >> user;
		if (user.empty()) {
			return {"", "", ""};
		}
	}

	if (pass.empty()) {
		if (!pass_file.empty()) {
			pass = read_password(pass_file);
		} else if (!pass_eval.empty()) {
			pass = eval_password(pass_eval);
		} else {
			if (!flushed) {
				std::cout << std::endl;
				std::cout.flush();
			}
			// Find a way to do this in C++ by removing cin echoing.
			std::string pass_prompt = "Password for " + name + ": ";
			pass = std::string(getpass(pass_prompt.c_str()));
		}

		if (pass.empty()) {
			return {"", "", ""};
		}
	}

	return {user, pass, token};
}

void RemoteApi::update_flag(const std::string& oldflags,
	const std::string& newflags,
	char flag, std::function<void(bool added)>&& do_update)
{
	const bool old_has = oldflags.find(flag) != std::string::npos;
	const bool new_has = newflags.find(flag) != std::string::npos;
	if (!old_has && new_has) {
		do_update(true);
	} else if (old_has && !new_has) {
		do_update(false);
	}
}

} // namespace newsboat
