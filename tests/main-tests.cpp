#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "tests.h"

using namespace std;

int main(int /*argc*/, char** /*argv*/) {
	auto path = std::filesystem::current_path();
	std::filesystem::current_path(path.parent_path());
	std::cout << "CD:" << path.string() << std::endl;

	std::cout << "Running main() from " << __FILE__ << std::endl;

	const std::string cfg_fpth{ "config/config-core.json" };

	sh::tests::TESTER_Somhunter::run_all_tests(cfg_fpth);
	// TODO update config tests
	// sh::tests::TESTER_Config::run_all_tests(cfg_fpth);

	return 0;
}
