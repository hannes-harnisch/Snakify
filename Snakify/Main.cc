#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace std;
using namespace std::filesystem;

regex const camel_case("[^A-Za-z][a-z]+([A-Z][a-z]*)+");

vector<string_view> split(string_view str, string_view delimiter)
{
	vector<string_view> tokens;

	size_t offset = 0, occurrence = 0;
	while(occurrence != string_view::npos)
	{
		occurrence = str.find(delimiter, offset);
		tokens.emplace_back(str.substr(offset, occurrence - offset));
		offset = occurrence + delimiter.length();
	}

	return tokens;
}

string_view extract_arg(vector<string_view> const& args, string_view arg_begin, string error_msg)
{
	auto extensions_arg = find_if(args.begin(), args.end(), [&](auto arg) {
		return arg.starts_with(arg_begin);
	});

	if(extensions_arg == args.end())
		throw runtime_error(error_msg);

	return extensions_arg->substr(extensions_arg->find_first_of('=') + 1);
}

string snakify_token(string_view token)
{
	string result;
	result.reserve(token.size());

	for(char ch : token)
	{
		if(!isupper(ch))
			result += ch;
		else
			(result += '_') += static_cast<char>(tolower(ch));
	}

	return result;
}

void snakify_text(string const& text, ofstream& output)
{
	sregex_token_iterator tokens(text.begin(), text.end(), camel_case, 0);
	sregex_token_iterator tokens_end;

	auto cur_txt_begin = text.begin();
	for_each(tokens, tokens_end, [&](ssub_match const& token) {
		string_view match(token.first, token.second);
		output << string_view(cur_txt_begin, token.first) << snakify_token(match);
		cur_txt_begin = token.second;
	});
	output << string_view(cur_txt_begin, text.end());
}

void snakify_all_files(string_view input_path, vector<string_view> const& extensions, string_view output_path_str)
{
	static string_view orig_input_path;
	if(orig_input_path.empty())
		orig_input_path = input_path;

	for(directory_iterator entry(input_path); entry != directory_iterator(); ++entry)
	{
		auto path_str = entry->path().string();
		if(entry->is_directory())
			snakify_all_files(path_str, extensions, output_path_str);

		if(!entry->is_regular_file())
			continue;

		// Check if current directory entry is a file with one of our desired extensions
		auto cur_extension = entry->path().extension().string();
		cur_extension.erase(cur_extension.begin()); // Remove leading dot

		auto extension = find(extensions.begin(), extensions.end(), cur_extension);
		if(extension == extensions.end())
			continue;

		ifstream input(path_str);
		if(!input)
			throw runtime_error("Could not open input file: " + path_str);

		string_view subpath_str(path_str.begin() + orig_input_path.size() + 1, path_str.end());
		auto		output_path = output_path_str / path(subpath_str);
		auto		new_dir		= output_path.parent_path();

		if(!exists(new_dir))
			create_directories(new_dir);

		ofstream output(output_path);
		if(!output)
			throw runtime_error("Could not open output file: " + output_path.string());

		string text(istreambuf_iterator<char>(input), {});
		snakify_text(text, output);
	}
}

void snakify(vector<string_view> const& args)
{
	if(args.empty())
		throw runtime_error("No command line arguments.");

	auto extensions_str = extract_arg(args, "-extensions=", "No extensions were specified. Example: -extensions=cc,cpp,hh,hpp");
	auto input_path		= extract_arg(args, "-inputdir=", "No input directory was specified. Example: -inputdir=../MyProject");
	auto output_path_str = extract_arg(args, "-outputdir=",
									   "No output directory was specified. Example: -outputdir=../MyConvertedProject");

	auto extensions = split(extensions_str, ",");

	snakify_all_files(input_path, extensions, output_path_str);
}

int main(int argc, char* argv[])
{
	vector<string_view> args(argv + 1, argv + argc);

#if DEBUG
	args = {"-inputdir=../../../TestInput", "-outputdir=../../../TestOutput", "-extensions=cc"};
#endif

	try
	{
		snakify(args);
	}
	catch(std::exception const& e)
	{
		cout << "ERROR: " << e.what();
	}
}
