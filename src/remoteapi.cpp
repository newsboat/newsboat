#include "remoteapi.h"

#include <fstream>
#include <glob.h>
#include <iostream>
#include <unistd.h>

#include "utils.h"

namespace newsboat {

const std::string RemoteApi::read_password(const std::string& file)
{
	glob_t exp;
	std::ifstream ifs;
	std::string pass = "";

	int res = glob(file.c_str(), GLOB_ERR, nullptr, &exp);
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
	std::string user = cfg->get_configvalue(scope + "-login");
	std::string pass = cfg->get_configvalue(scope + "-password");
	std::string pass_file = cfg->get_configvalue(scope + "-passwordfile");
	std::string pass_eval = cfg->get_configvalue(scope + "-passwordeval");

	bool flushed = false;

	if (user.empty()) {
		std::cout << std::endl;
		std::cout.flush();
		flushed = true;
		printf("Username for %s: ", name.c_str());
		std::cin >> user;
		if (user.empty()) {
			return {"", ""};
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
			return {"", ""};
		}
	}

	return {user, pass};
}

} // namespace newsboat
