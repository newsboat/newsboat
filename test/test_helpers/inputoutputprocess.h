#ifndef NEWSBOAT_TEST_HELPERS_INPUTOUTPUTPROCESS_H_
#define NEWSBOAT_TEST_HELPERS_INPUTOUTPUTPROCESS_H_

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

namespace test_helpers {

class Pipe {
public:
	Pipe();
	~Pipe();

	Pipe(const Pipe&) = delete;
	Pipe& operator=(const Pipe&) = delete;

	void close_read_side();
	void close_write_side();
	int read_side();
	int write_side();

private:
	int handles[2];
};

class InputOutputProcess {
public:
	InputOutputProcess(std::vector<std::string>&& args);
	~InputOutputProcess();

	InputOutputProcess(const InputOutputProcess&) = delete;
	InputOutputProcess& operator=(const InputOutputProcess&) = delete;

	std::vector<std::uint8_t> read_binary(std::size_t length);
	std::string read_line();
	void write_binary(const std::uint8_t* data, std::size_t length);
	void write_line(std::string data);

	void wait_for_exit();

private:
	void start_process();

	const std::vector<std::string> argsStorage;
	std::vector<char*> args;
	std::vector<char*> envVars;
	Pipe stdinPipe;
	Pipe stdoutPipe;
	FILE* stdoutStream;
	int childPid;
};

} // namespace test_helpers

#endif /* NEWSBOAT_TEST_HELPERS_INPUTOUTPUTPROCESS_H_ */
