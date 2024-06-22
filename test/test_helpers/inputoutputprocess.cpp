#include "inputoutputprocess.h"

#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <stdlib.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <errno.h>

namespace test_helpers {

Pipe::Pipe()
{
	const auto result = pipe(handles);
	if (result < 0) {
		throw std::runtime_error("Failed to create pipe");
	}
}

Pipe::~Pipe()
{
	close_read_side();
	close_write_side();
}

void Pipe::close_read_side()
{
	if (handles[0] != -1) {
		close(handles[0]);
		handles[0] = -1;
	}
}

void Pipe::close_write_side()
{
	if (handles[1] != -1) {
		close(handles[1]);
		handles[1] = -1;
	}
}

int Pipe::read_side()
{
	return handles[0];
}

int Pipe::write_side()
{
	return handles[1];
}

InputOutputProcess::InputOutputProcess(std::vector<std::string>&& arguments)
	: argsStorage(arguments)
{
	for (const auto& arg : argsStorage) {
		args.push_back(const_cast<char*>(arg.c_str()));
	}
	args.push_back(nullptr);
	envVars.push_back(nullptr);

	start_process();
	stdoutStream = fdopen(stdoutPipe.read_side(), "r");
	if (stdoutStream == nullptr) {
		throw std::system_error(errno, std::generic_category());
	}
}

InputOutputProcess::~InputOutputProcess()
{
	fclose(stdoutStream);
}

// Based on: https://stackoverflow.com/questions/9405985/linux-executing-child-process-with-piped-stdin-stdout
void InputOutputProcess::start_process()
{
	childPid = fork();
	if (childPid == 0) {
		// We are the child process

		// redirect stdin
		if (dup2(stdinPipe.read_side(), STDIN_FILENO) == -1) {
			exit(errno);
		}

		// redirect stdout
		if (dup2(stdoutPipe.write_side(), STDOUT_FILENO) == -1) {
			exit(errno);
		}

		// all these are for use by parent only
		stdinPipe.close_read_side();
		stdinPipe.close_write_side();
		stdoutPipe.close_read_side();
		stdoutPipe.close_write_side();

		// run child process image
		auto result = execve(args[0], args.data(), envVars.data());

		// if we get here at all, an error occurred, but we are in the child process, so just exit
		exit(result);
	} else if (childPid > 0) {
		// We are the parent process

		stdinPipe.close_read_side();
		stdoutPipe.close_write_side();
	} else {
		throw std::runtime_error("Failed to fork");
	}
}

std::vector<std::uint8_t> InputOutputProcess::read_binary(std::size_t length)
{
	std::vector<std::uint8_t> buffer;
	buffer.resize(length);
	const auto bytesRead = fread(buffer.data(), buffer.size(), 1, stdoutStream);
	if (bytesRead != length) {
		throw std::runtime_error("Failed to read binary data from pipe");
	}

	return buffer;
}

std::string InputOutputProcess::read_line()
{
	char buffer[1024];
	if (fgets(buffer, sizeof(buffer), stdoutStream) == nullptr) {
		throw std::system_error(errno, std::generic_category());
	}
	auto input = std::string(buffer);
	if (input.length() > 0) {
		// Remove newline character
		input.resize(input.size() - 1);
	}
	return input;
}

void InputOutputProcess::write_binary(const std::uint8_t* data, std::size_t length)
{
	const std::uint8_t* current = data;
	std::size_t remaining = length;
	while (remaining > 0) {
		const auto bytes_written = write(stdinPipe.write_side(), current, remaining);
		if (bytes_written < 0) {
			throw std::system_error(errno, std::generic_category());
		}
		current += bytes_written;
		remaining -= bytes_written;
	}
}

void InputOutputProcess::write_line(std::string data)
{
	data += "\n";

	write_binary(reinterpret_cast<const std::uint8_t*>(data.c_str()), data.length());
}

void InputOutputProcess::wait_for_exit()
{
	int status;
	waitpid(childPid, &status, 0);
}

} // namespace test_helpers
