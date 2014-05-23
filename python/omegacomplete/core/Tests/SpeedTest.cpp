#include "stdafx.hpp"

#include "Omegacomplete.hpp"
#include "Stopwatch.hpp"

using namespace boost;
using namespace boost::filesystem;

int main() {
  Stopwatch watch;

  Omegacomplete::InitStatic();
  Omegacomplete* omegacomplete = new Omegacomplete;

  unsigned int counter = 1;
  std::string contents;
  std::string command;
  std::string resp;

  uint64_t ns = 0;

#ifdef _WIN32
  AUTO(iter, directory_iterator(path("C:/SVN/Syandus_ALIVE4/Platform/Source/Code/SyCore")));
#else
  AUTO(iter, directory_iterator(path("/home/rko/SVN/Syandus_ALIVE4/Platform/Source/Code/SyCore")));
#endif
  for (; iter != directory_iterator(); ++iter) {
    path item = iter->path();
    std::string filename = item.generic_string();
    if (ends_with(filename, ".h") || ends_with(filename, ".cpp")) {
      std::ifstream t(filename.c_str());
      t.seekg(0, std::ios::end);   
      contents.reserve(t.tellg());
      t.seekg(0, std::ios::beg);

      contents.assign((std::istreambuf_iterator<char>(t)),
                      std::istreambuf_iterator<char>());

      watch.Start();

      command = "current_buffer_id " + lexical_cast<std::string>(counter);
      counter++;
      resp = omegacomplete->Eval(command);
      //std::cout << resp << "\n";

      command = "current_buffer_absolute_path " + filename;
      resp = omegacomplete->Eval(command);
      //std::cout << resp << "\n";

      command = "buffer_contents_follow 1";
      resp = omegacomplete->Eval(command);
      //std::cout << resp << "\n";

      resp = omegacomplete->Eval(contents);
      //std::cout << resp << "\n";

      ns += watch.StopResult();
    }
  }

  double d;

  d = (double)ns / 1e6;
  std::cout << "sent files in " << d << " ms\n";

  watch.Start();
  command = "free_buffer " + lexical_cast<std::string>(0xFFFFFFF);
  resp = omegacomplete->Eval(command);
  ns += watch.StopResult();

  d = (double)ns / 1e6;
  std::cout << "parsed files in " << d << " ms\n";

  std::vector<std::string> tests{
    "seg",
    "BASS_",
    "Play",
    "sfm",
    "VoidCall",
    "rffl",
    "sem",
    "SetFra",
    "Get",
    "Set",
  };

  std::vector<std::string> results;

  ns = 0;
  for (size_t i = 0; i < tests.size(); ++i) {
    std::string line = tests[i];

    watch.Start();
    command = "complete " + line;
    resp = omegacomplete->Eval(command);
    ns += watch.StopResult();

    results.push_back(resp);
  }

  d = (double)ns / 1e6;
  std::cout << lexical_cast<std::string>(tests.size()) << " completions ran in in " << d << " ms\n";

  for (size_t i = 0; i < results.size(); ++i) {
    std::cout << results[i] << "\n\n";
  }

  delete omegacomplete;
  omegacomplete = NULL;

  return 0;
}
