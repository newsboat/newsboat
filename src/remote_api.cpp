#include <fstream>
#include <iostream>
#include <unistd.h>
#include <wordexp.h>

#include <remote_api.h>
#include <utils.h>

namespace newsbeuter {

const std::string remote_api::read_password(const std::string& file) {
	wordexp_t exp;
	std::ifstream ifs;
	std::string pass = "";

	wordexp(file.c_str(), &exp, 0);
	if (exp.we_wordc > 0) {
		ifs.open(exp.we_wordv[0]);
	}
	wordfree(&exp);
	if (ifs.is_open()) {
		std::getline(ifs, pass);
	}

	return pass;
}

const std::string remote_api::eval_password(const std::string& cmd) {
	std::string pass = utils::get_command_output(cmd);
	std::string::size_type pos = pass.find_first_of("\n\r");

	if (pos != std::string::npos) {
		pass.resize(pos);
	}

	return pass;
}

credentials remote_api::get_credentials(const std::string& scope, const std::string& name) {
	std::string user = cfg->get_configvalue(scope+"-login");
	std::string pass = cfg->get_configvalue(scope+"-password");
	std::string pass_file = cfg->get_configvalue(scope+"-passwordfile");
	std::string pass_eval = cfg->get_configvalue(scope+"-passwordeval");

	bool flushed = false;

	if (user.empty()) {
		std::cout << std::endl;
		std::cout.flush();
		flushed = true;
		printf("Username for %s: ", name.c_str());
		std::cin >> user;
		if (user.empty()) {
			return { "", "" };
		}
	}

	if (pass.empty()) {
		if (!pass_file.empty()) {
			pass = read_password(pass_file);
			if (pass.empty()) return { "", "" };
		} else if (!pass_eval.empty()) {
			pass = eval_password(pass_eval);
			if (pass.empty()) return { "", "" };
		}
	}

	if (pass.empty()) {
		if (!flushed) {
			std::cout << std::endl;
			std::cout.flush();
  		}
		// Find a way to do this in C++ by removing cin echoing.
		std::string pass_prompt = "Password for "+name+": ";
		pass = std::string(getpass(pass_prompt.c_str()));
	}

	return { user, pass };
}

}
