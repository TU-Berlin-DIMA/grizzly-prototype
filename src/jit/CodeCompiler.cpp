#include <jit/CodeCompiler.hpp>

#include <boost/filesystem/operations.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <fstream>
#include <sstream>
#include <utility>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

#include <assert.h>
#include <dlfcn.h>
#include <iostream>

#pragma GCC diagnostic pop

const std::string CCodeCompiler::IncludePath = "jit-generated-code/";
const std::string CLANG_EXECUTABLE = "/usr/bin/clang++";

const std::string CCodeCompiler::MinimalApiHeaderPath = "/include/CodeGen/MinimalApi.hpp";

CCodeCompiler::CCodeCompiler() { init(); }

CompiledCCodePtr CCodeCompiler::compile(const std::string &source, const std::string name) {
  // handleDebugging(source);
  // auto pch_time = createPrecompiledHeader();

  return compileWithSystemCompiler(source, 0, name);
}

void CCodeCompiler::init() {
  use_clang_jit_ = false;

  show_generated_code_ = true;

  debug_code_generator_ = true;

  keep_last_generated_query_code_ = false;

#ifndef NDEBUG
  PrecompiledHeaderName = ".debug.hpp.pch";
#else
  PrecompiledHeaderName = ".release.hpp.pch";
#endif
  initCompilerArgs();
}

void CCodeCompiler::initCompilerArgs() {
  compiler_args_ = {"-std=c++11",      "-O3",   "-ltbb",   "-lnuma",
                    "-fno-trigraphs",  "-fpic", "-Werror", "-Wparentheses-equality",
#ifdef SSE41_FOUND
                    "-msse4.1",
#endif
#ifdef SSE42_FOUND
                    "-msse4.2",
#endif
#ifdef AVX_FOUND
                    "-mavx",
#endif
#ifdef AVX2_FOUND
                    "-mavx2",
#endif
                    "-I" + IncludePath};

#ifndef NDEBUG
  compiler_args_.push_back("-g");
#else
  compiler_args_.push_back("-O3");
  compiler_args_.push_back("-g");
#endif
}

long CCodeCompiler::createPrecompiledHeader() {
  if (!rebuildPrecompiledHeader()) {
    return 0;
  }

  auto start = 0;
  callSystemCompiler(getPrecompiledHeaderCompilerArgs());
  return 0 - start;
}

bool CCodeCompiler::rebuildPrecompiledHeader() {
  if (!boost::filesystem::exists(PrecompiledHeaderName)) {
    return true;
  } else {
    auto last_access_pch = boost::filesystem::last_write_time(PrecompiledHeaderName);
    auto last_access_header = boost::filesystem::last_write_time(MinimalApiHeaderPath);

    /* pre-compiled header outdated? */
    return last_access_header > last_access_pch;
  }
}

std::vector<std::string> CCodeCompiler::getPrecompiledHeaderCompilerArgs() {
  auto args = compiler_args_;

  std::stringstream pch_option;
  pch_option << "-o" << PrecompiledHeaderName;
  args.push_back(MinimalApiHeaderPath);
  args.push_back(pch_option.str());
  args.push_back("-xc++-header");

  return args;
}

std::vector<std::string> CCodeCompiler::getCompilerArgs() {
  auto args = compiler_args_;

  args.push_back("-xc++");
#ifndef NDEBUG
  // args.push_back("-include.debug.hpp");
#else
  // args.push_back("-include.release.hpp");
#endif

#ifdef __APPLE__
  args.push_back("-framework OpenCL");
  args.push_back("-undefined dynamic_lookup");
#endif

  return args;
}

void CCodeCompiler::callSystemCompiler(const std::vector<std::string> &args) {
  std::stringstream compiler_call;
  compiler_call << CLANG_EXECUTABLE << " ";

  for (const auto &arg : args) {
    compiler_call << arg << " ";
  }
  std::cout << "system '" << compiler_call.str() << "'" << std::endl;
  auto ret = system(compiler_call.str().c_str());

  if (ret != 0) {
    std::cout << "PrecompiledHeader compilation failed!";
    throw "PrecompiledHeader compilation failed!";
  }
}

void pretty_print_code(const std::string &source) {
  int ret = system("which clang-format > /dev/null");
  if (ret != 0) {
    std::cout << "Did not find external tool 'clang-format'. "
                 "Please install 'clang-format' and try again."
                 "If 'clang-format-X' is installed, try to create a "
                 "symbolic link.";
    return;
  }
  const std::string filename = "temporary_file.c";

  exportSourceToFile(filename, source);

  std::string format_command = std::string("clang-format ") + filename;
  /* try a syntax highlighted output first */
  /* command highlight available? */
  ret = system("which highlight > /dev/null");
  if (ret == 0) {
    format_command += " | highlight --src-lang=c -O ansi";
  }
  ret = system(format_command.c_str());
  std::string cleanup_command = std::string("rm ") + filename;
  ret = system(cleanup_command.c_str());
}

void CCodeCompiler::handleDebugging(const std::string &source) {
  if (!show_generated_code_ && !debug_code_generator_ && !keep_last_generated_query_code_) {
    return;
  }

  if (keep_last_generated_query_code_ || debug_code_generator_) {
    exportSourceToFile("last_generated_query.c", source);
  }

  if (show_generated_code_ || debug_code_generator_) {
    std::cout << std::string(80, '=') << std::endl;
    std::cout << "<<< Generated Host Code:" << std::endl;
    pretty_print_code(source);
    std::cout << ">>> Generated Host Code" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
  }
}

void exportSourceToFile(const std::string &filename, const std::string &source) {
  std::ofstream result_file(filename, std::ios::trunc | std::ios::out);
  result_file << source;
}

class SystemCompilerCompiledCCode : public CompiledCCode {
public:
  SystemCompilerCompiledCCode(long compile_time, SharedLibraryPtr library, const std::string &base_name)
      : CompiledCCode(compile_time), library_(library), base_file_name_(base_name) {}

  ~SystemCompilerCompiledCCode() { cleanUp(); }

protected:
  void *getFunctionPointerImpl(const std::string &name) override final { return library_->getSymbol(name); }

private:
  void cleanUp() {
    if (boost::filesystem::exists(base_file_name_ + ".cpp")) {
      boost::filesystem::remove(base_file_name_ + ".cpp");
    }

    if (boost::filesystem::exists(base_file_name_ + ".o")) {
      boost::filesystem::remove(base_file_name_ + ".o");
    }

    if (boost::filesystem::exists(base_file_name_ + ".so")) {
      boost::filesystem::remove(base_file_name_ + ".so");
    }

    if (boost::filesystem::exists(base_file_name_ + ".c.orig")) {
      boost::filesystem::remove(base_file_name_ + ".c.orig");
    }
  }

  SharedLibraryPtr library_;
  std::string base_file_name_;
};

CompiledCCodePtr CCodeCompiler::compileWithSystemCompiler(const std::string &source, const long pch_time,
                                                          const std::string name) {
  auto start = 0;

  boost::uuids::uuid uuid = boost::uuids::random_generator()();
  std::string basename = "jit-generated-code/gen_query_" + name;
  std::string filename = basename + ".cpp";
  std::string library_name = basename + ".so";
  exportSourceToFile(filename, source);

  auto args = getCompilerArgs();
  args.push_back("--shared");
  args.push_back("-o" + library_name);
  args.push_back(filename);

  callSystemCompiler(args);

  auto shared_library = SharedLibrary::load("./" + library_name);

  auto end = 0;

  auto compile_time = end - start + pch_time;
  return std::make_shared<SystemCompilerCompiledCCode>(compile_time, shared_library, basename);
}

SharedLibrary::SharedLibrary(void *_shared_lib) : shared_lib_(_shared_lib) { assert(shared_lib_ != NULL); }

SharedLibrary::~SharedLibrary() {
  //  if (!VariableManager::instance().getVariableValueBoolean(
  //          "profiling.keep_shared_libraries_loaded")) {
  dlclose(shared_lib_);
  //}
}

void *SharedLibrary::getSymbol(const std::string &mangeled_symbol_name) const {
  auto symbol = dlsym(shared_lib_, mangeled_symbol_name.c_str());
  auto error = dlerror();

  if (error) {
    std::cout << "Could not load symbol: " << mangeled_symbol_name << std::endl << "Error:" << std::endl << error;
  }

  return symbol;
}

SharedLibraryPtr SharedLibrary::load(const std::string &file_path) {
  auto myso = dlopen(file_path.c_str(), RTLD_NOW);

  auto error = dlerror();
  if (error) {
    std::cout << "Could not load shared library: " << file_path << std::endl << "Error:" << std::endl << error;
  } else if (!myso) {
    std::cout << "Could not load shared library: " << file_path << std::endl << "Error unknown!";
  }

  return SharedLibraryPtr(new SharedLibrary(myso));
}
