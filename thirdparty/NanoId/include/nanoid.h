#pragma once

#include "crypto_random.h"
#include "settings.h"
#include <future>
#include <random>
#include <string>

namespace NANOID_NAMESPACE
{
	std::string generate();
	std::string generate(const std::string& alphabet);
	std::string generate(std::size_t size);
	std::string generate(const std::string& alphabet, std::size_t size);

	std::future<std::string> generate_async();
	std::future<std::string> generate_async(const std::string& alphabet);
	std::future<std::string> generate_async(std::size_t size);
	std::future<std::string> generate_async(const std::string& alphabet, std::size_t size);

	std::string generate(crypto_random_base& random);
	std::string generate(crypto_random_base& random, const std::string& alphabet);
	std::string generate(crypto_random_base& random, std::size_t size);
	std::string generate(crypto_random_base& random, const std::string& alphabet, std::size_t size);
}