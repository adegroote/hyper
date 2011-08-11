#include <hyperConfig.hh>

#include <iostream>
#include <fstream>
#include <set>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <compiler/depends.hh>
#include <compiler/extension.hh>
#include <compiler/parser.hh>
#include <compiler/universe.hh>
#include <compiler/utils.hh>
#include <compiler/task.hh>
#include <compiler/task_parser.hh>
#include <compiler/recipe.hh>
#include <compiler/recipe_parser.hh>

#include <dlfcn.h>

using namespace hyper::compiler;
using namespace boost::filesystem;
namespace po = boost::program_options;

void
usage(const po::options_description& desc)
{
	std::cout << "Usage: hyperc [options]\n";
	std::cout << desc;
}

void build_main(std::ostream& oss, const std::string& name)
{
	std::string main = 
		"#include <iostream>\n"
		"#include <model/main.hh>\n"
		"#include <@NAME@/ability.hh>\n"
		"\n"
		"int main(int argc, char** argv)\n"
		"{\n"
		"	return hyper::model::main<hyper::@NAME@::ability>(argc, argv, \"@NAME@\");\n"
		"}\n"
	;

	oss << hyper::compiler::replace_by(main, "@NAME@", name);
}

void build_swig(std::ostream& oss, const std::string& name)
{
	std::string swig = 
		"%module @NAME@\n"
		"%include \"typemaps.i\"\n"
		"%{\n"
		"	#include <string>\n"
		"	#include \"@NAME@/export.hh\"\n"
		"	#include \"@NAME@/types.hh\"\n"
		"	#include <model/future.hh>\n"
		"	using namespace hyper::@NAME@;\n"
		"%}\n"
		"%include \"std_string.i\"\n"
		"%include \"@NAME@/types.hh\"\n"
		"%include <model/future.hh>\n"
		"%include \"@NAME@/export.hh\"\n"
		"%template(Future_bool) hyper::model::future_value<bool>;\n"
		;

	oss << hyper::compiler::replace_by(swig, "@NAME@", name);

}

void build_base_cmake(std::ostream& oss, const std::string& name,
					  const std::set<std::string>& depends)
{
	std::string base_cmake = 
		"cmake_minimum_required(VERSION 2.6 FATAL_ERROR)\n"
		"\n"
		"find_package(Hyper REQUIRED)\n"
		"\n"
		"hyper_node(@NAME@)\n"
		"\n"
		;

	std::string s;
	if (depends.empty())
		s = name;
	else {
		std::ostringstream oss;
		oss << name << " REQUIRES ";
		std::copy(depends.begin(), depends.end(), std::ostream_iterator<std::string>(oss, " "));
		s = oss.str();
	}

	oss << hyper::compiler::replace_by(base_cmake, "@NAME@", s);
}

/*
 * Assume src always exists
 */
bool are_file_equals(const path& src, const path& dst)
{
	if (!exists(dst)) 
		return false;

	std::string src_content = read_from_file(src.string());
	std::string dst_content = read_from_file(dst.string());

	return (src_content == dst_content);
}

void copy_if_different(const path& file_in, const path& file_out)
{
	if (!are_file_equals(file_in, file_out)) {
		std::cout << "copying " << file_in << " to " << file_out << std::endl;
		// XXX there is to have a bug when overwriting files, so remove
		// the file_out before copying the new one
		if (exists(file_out))
			remove(file_out);
		copy_file(file_in, file_out, copy_option::overwrite_if_exists);
	}
}

void copy_if_different(const path& base_src, const path& base_dst,
					   const path& current_path)
{
	path src = base_src / current_path;
	path dst = base_dst / current_path;

	create_directory(dst);

	directory_iterator end_itr; 
	for (directory_iterator itr( src ); itr != end_itr; ++itr ) {
		if (is_regular_file(itr->path())) {
			path src_file = src / itr->path().filename();
			path dst_file = dst / itr->path().filename();
			copy_if_different(src_file, dst_file);
		} else if (is_directory(itr->path())) {
			path current = current_path / itr->path().filename();
			copy_if_different(base_src, base_dst, current);
		}
	}
}

bool is_directory_empty(const path& directory)
{
	directory_iterator end_itr;
	directory_iterator itr(directory);
	return (itr != end_itr);
}

void symlink_all(const path& src, const path& dst)
{
	directory_iterator end_itr; 
	for (directory_iterator itr( src ); itr != end_itr; ++itr ) {
		path src_file = src/itr->path().filename();
		path dst_file = dst/itr->path().filename();
		if (!exists(dst_file)) {
			create_symlink(complete(src_file), complete(dst_file));
		}
	}
}

struct generate_recipe 
{
	const universe& u;
	const typeList& tList;
	ability& ab;
	task& t;
	depends& deps;

	bool& success;
	const recipe_context_decl& context;
	std::string directoryName;

	generate_recipe(universe& u_,
					const std::string& abilityName, 
					const std::string& taskName,
					const std::string& directoryName_,
					depends& deps,
					bool& success,
					const recipe_context_decl& context) :
		u(u_), ab(u_.get_ability(abilityName)), tList(u.types()), 
		t(ab.get_task(taskName)), deps(deps),
		success(success), context(context), directoryName(directoryName_) 
	{}

	void operator() (const recipe_decl& decl)
	{
		recipe r(decl, context, ab, t, tList);
		bool valid = r.validate(u);
		if (!valid) {
			std::cerr << "Recipe " << r.get_name() << " seems not valid " << std::endl;
		}
		success = success && valid;
		success = success && t.add_recipe(r);
		if (success) {
			r.add_depends(deps, u);
			{
			std::string fileName = directoryName + "/" + r.get_name() + ".cc";
			std::ofstream oss(fileName.c_str());
			r.dump(oss, u);
			}
			{
			std::string fileName = directoryName + "/" + r.get_name() + ".hh";
			std::ofstream oss(fileName.c_str());
			r.dump_include(oss, u);
			}
		}
	}
};

struct add_task
{
	const universe& u;
	ability& ab;
	const typeList& tList;
	depends& deps;

	bool& success;

	add_task(universe& u_,
				  const std::string& abilityName,
				  depends& deps, bool& success) :
		u(u_), ab(u_.get_ability(abilityName)), tList(u.types()),
		deps(deps), success(success)
	{}

	void operator() (const task_decl& decl)
	{
		task t(decl, ab, tList);
		bool valid = t.validate(u);
		if (!valid) {
			std::cerr << "Task " << t.get_name() << " seems not valid " << std::endl;
		}
		valid &= ab.add_task(t);
		success = success && valid;
		if (valid) 
			t.add_depends(deps, u);
	}
};

struct generate_task
{
	const universe& u;
	std::string directoryName;

	generate_task(universe& u_,
				  const std::string& directoryName_) :
		u(u_), directoryName(directoryName_)
	{}

	void operator() (const task& t)
	{
		{
			std::string fileName = directoryName + "/" + t.get_name() + ".cc";
			std::ofstream oss(fileName.c_str());
			t.dump(oss, u);
		}
		{
			std::string fileName = directoryName + "/" + t.get_name() + ".hh";
			std::ofstream oss(fileName.c_str());
			t.dump_include(oss, u);
		}
	}
};

static int
clean_ability(const std::string& abilityName)
{
	(void) abilityName;
	remove_all("src");
	remove("CMakeLists.txt");
	remove_all(".hyper");
	return 0;
}

struct close_handle
{
	void operator() (const std::pair<std::string, void*>& p) const
	{
		dlclose(p.second);
	}
};

class extension_manager 
{
	typedef std::map<std::string, void*> extension_map;
	extension_map extension_handle;
	universe& u;

	public:
		extension_manager(universe& u) : u(u) {}

		// XXX better handle error
		void load_extension(const std::string& str)
		{
			extension_map::const_iterator it = extension_handle.find(str);
			if (it != extension_handle.end())
				return;

			std::string libname = "libhyper_" + str + ".so";
			void* handle = dlopen(libname.c_str(), RTLD_NOW|RTLD_GLOBAL);
			char* error;
			typedef hyper::compiler::extension* (*ptr_ext) ();
		
			if (!handle) {
				std::cerr << "Failed to load " << str << " : " << dlerror();
				exit(EXIT_FAILURE);
			}
		
			dlerror(); // clear any existing error
		
			ptr_ext init = (ptr_ext) dlsym(handle, "init");
		
			if ((error = dlerror()) != NULL) {
				std::cerr << "Failed to load init symbol : " << error;
				exit(EXIT_FAILURE);
			}
		
			u.add_extension(str, init());
		}

		~extension_manager() {
			std::for_each(extension_handle.begin(),
						  extension_handle.end(),
						  close_handle());
		}
};

int main(int argc, char** argv)
{
	try {
	po::options_description desc("Allowed options");
	desc.add_options()
		("clean,c", "clean generated files")
		("help,h", "produce help message")
		("verbose,v", po::value<int>()->implicit_value(1),
		 "enable verbosity (optionally specify level)")
		("include-path,I", po::value< std::vector<std::string> >(), 
		 "include path")
		("extension,e", po::value <std::vector<std::string> >(),
		 "extension")
		("initial,i", "initial files for user_defined function / type")
		("input-file", po::value< std::string >(), "input file")
		;

	po::positional_options_description p;
	p.add("input-file", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).
			options(desc).positional(p).run(), vm);
	po::notify(vm);

	if (vm.count("help")) {
		usage(desc);
		return 0;
	}

	if (!vm.count("input-file"))
	{
		usage(desc);
		return -1;
	}

	std::vector<path> include_dir_path;
	if (vm.count("include-path")) {
		std::vector<std::string> include_dirs = vm["include-path"].as < std::vector<std::string> >();
		std::copy(include_dirs.begin(), include_dirs.end(), std::back_inserter(include_dir_path));
	}


	bool initial = (vm.count("initial") != 0);

	std::string abilityName = vm["input-file"].as < std::string > ();

	if (vm.count("clean")) {
		return clean_ability(abilityName);
	}

	std::string baseName = ".hyper/src/";
	std::string baseUserName = ".hyper/user_defined";
	std::string directoryName = baseName + abilityName;
	std::string directoryTaskName = directoryName + "/tasks";
	std::string directoryRecipeName = directoryName + "/recipes";
	std::string taskDirectory = abilityName;

	universe u;
	extension_manager manager(u);

	if (vm.count("extension")) {
		std::vector<std::string> extensions_to_load = vm["extension"].as < std::vector<std::string> >();
		std::for_each(extensions_to_load.begin(), extensions_to_load.end(),
						boost::bind(&extension_manager::load_extension, manager, _1));
	}

	parser P(u, include_dir_path);

	create_directory(".hyper");
	create_directory(baseName);
	create_directory(baseUserName);
	create_directory(directoryName);	
	create_directory(directoryTaskName);
	create_directory(directoryRecipeName);

	bool res = P.parse_ability_file(abilityName + ".ability");
	if (res == false)
		return -1;

	const ability& current_a = u.get_ability(abilityName);
	depends deps;

	if (exists(taskDirectory)) {
		directory_iterator end_itr; // default construction yields past-the-end

		// parse and generate all tasks
		for (directory_iterator itr( taskDirectory ); itr != end_itr; ++itr ) {
			if (is_regular_file(itr->path())) {
				task_decl_list task_decls;
				res = P.parse_task_file(itr->path().string(), task_decls);
				if (res == false) {
					std::cerr << "Fail to parse " << itr->path().string() << std::endl;
					return -1;
				}

				bool success = true;
				add_task adder(u, abilityName, deps, success);
				std::for_each(task_decls.list.begin(), 
						task_decls.list.end(),
						adder);
				if (success == false)
					return -1;
			} 
		} 


		// now, enter in sub-directories to generate all recipes
		for (directory_iterator itr( taskDirectory ); itr != end_itr; ++itr ) {
			if (is_directory(itr->path())) {
#if BOOST_FILESYSTEM_VERSION == 3
				std::string taskName = itr->path().filename().string();
#elif BOOST_FILESYSTEM_VERSION == 2
				std::string taskName = itr->path().filename();
#endif
				directory_iterator end_itr2; 
				for (directory_iterator itr2(*itr); itr2 != end_itr2; ++itr2 ) {
					if (is_regular_file(itr2->path())) {
						recipe_decl_list rec_decls;
						res = P.parse_recipe_file(itr2->path().string(), rec_decls);
						if (res == false) {
							std::cerr << "Fail to parse " << itr2->path().string() << std::endl;
							return -1;
						}

						bool success = true;
						generate_recipe gen(u, abilityName, taskName, directoryRecipeName, 
											deps, success, rec_decls.context);
						std::for_each(rec_decls.recipes.begin(),
								rec_decls.recipes.end(),
								gen);
//						if (success == false)
//							return -1;
					}
				}
			}
		}
	}

	/* Generate task after recipe parsing because we need to know the list of
	 * recipes for each task
	 */
	std::for_each(current_a.task_begin(), current_a.task_end(), 
			generate_task(u, directoryTaskName));

	{
		std::string fileName = directoryName + "/types.hh";
		std::ofstream oss(fileName.c_str());
		u.dump_ability_types(oss, abilityName);
	}

	{
		u.dump_ability_opaque_types_def(baseUserName, abilityName);
	}

	bool define_func = false;
	{
		std::string fileName = directoryName + "/funcs.hh";
		std::ofstream oss(fileName.c_str());
		size_t basic_funcs = u.dump_ability_functions_proto(oss, abilityName);
		size_t tagged_funcs = u.dump_ability_tagged_functions_proto(baseUserName, abilityName);
		if ((basic_funcs + tagged_funcs)  == 0) {
			remove(fileName);
			define_func = false;
		} else {
			std::string directoryNameImpl = baseUserName + "/funcs/";
			create_directories(directoryNameImpl);
			u.dump_ability_functions_impl(directoryNameImpl, abilityName);
			define_func = true;
			{
				std::string fileName = directoryName + "/import.hh";
				std::ofstream oss(fileName.c_str());
				u.dump_ability_import_module_def(oss, abilityName);
			}

			{ 
				std::string fileName = directoryName + "/import.cc";
				std::ofstream oss(fileName.c_str());
				u.dump_ability_import_module_impl(oss, abilityName);
			}
		}
	}

	{
		std::string fileName = directoryName + "/ability.hh";
		std::ofstream oss(fileName.c_str());
		current_a.dump_include(oss, u);
	}

	{
		std::string fileName = directoryName + "/ability.cc";
		std::ofstream oss(fileName.c_str());
		current_a.dump(oss, u);
	}

	{
		std::string fileName = baseName + "main.cc";
		std::ofstream oss(fileName.c_str());
		build_main(oss, abilityName);
	}

	{
		std::string fileName = ".hyper/CMakeLists.txt";
		std::ofstream oss(fileName.c_str());
		std::set<std::string> d = current_a.get_type_depends(u.types());
		d.insert(deps.fun_depends.begin(), deps.fun_depends.end());

		// remove special case, need for empty namespace (base) and current
		// system (always link with itself)
		d.erase("");
		d.erase(current_a.name());
		build_base_cmake(oss, abilityName,  d);
	}

	{
		std::string fileName = directoryName + "/export.hh";
		std::ofstream oss(fileName.c_str());
		current_a.agent_export_declaration(oss, u.types());
	}
	{
		std::string fileName = directoryName + "/export.cc";
		std::ofstream oss(fileName.c_str());
		current_a.agent_export_implementation(oss, u.types());
	}
	{ 
		std::string fileName = baseName + abilityName + ".i";
		std::ofstream oss(fileName.c_str());
		build_swig(oss, abilityName);
		current_a.dump_swig_ability_types(oss, u.types());
	}

	/* Now for all files in .hyper/src, copy different one into real src */
	copy_if_different(baseName, "src", "");

	/* Copy CMakeLists.txt */
	copy_if_different(".hyper/CMakeLists.txt", "CMakeLists.txt");

	/* Copy user_defined template */
	directory_iterator end_itr; 
	if (exists(baseUserName) && initial) {
		if (exists("user_defined/")) {
			std::cerr << "Won't copy user_defined template, directory already existing!\n";
			std::cerr << "You can grab additional definition from .hyper/user_defined\n";
		} else {
			copy_if_different(baseUserName, "user_defined", "");
		}
	}

	/* Symlink between user_defined files and real impl */
	if (exists("user_defined"))
		symlink_all("user_defined/", "src/" + abilityName);

	} catch (std::exception &e) { 
		std::cerr << e.what() << std::endl;
		return -1;
	}

	return 0;
}
