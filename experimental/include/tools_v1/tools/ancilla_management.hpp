#ifndef ANCILLA_MANAGEMENT_HPP_
#define ANCILLA_MANAGEMENT_HPP_

#include <random>
#include <set>
#include <string>
#include <tools_v1/ast/base.hpp>
#include <tools_v1/tools/staq_builder.hpp>
#include <unordered_map>

namespace tools_v1 {
namespace tools {

class ANC_MEM {
  std::set<qbit> mem_;
  std::string cur_name;
  // the last index used corresponding to any of the names
  std::unordered_map<std::string, unsigned int> last_idx; 

public:
  ANC_MEM() = default;
  ~ANC_MEM() = default;

  qbit generate_ancilla(const std::string &prefix = "anc");

  // Get all ancillas for circuit saving
  const std::set<qbit> &get_all_ancillas() const { return mem_; }

  // // Save all ancillas to a circuit
  // void save_all_ancillas(circuit &c) const {
  //   for (const auto &anc : mem_) {
  //     c.save_ancilla(anc);
  //   }
  // }

  const std::unordered_map<std::string, unsigned int>& registers(){
    return last_idx;
  }
  

  // Clear all ancillas (for reset)
  void clear() {
    mem_.clear();
    last_idx.clear();
    cur_name.clear();
  }

  // Get count of ancillas
  size_t size() const { return mem_.size(); }

private:
  std::string gen_name();
  std::string rand_string(int length);
};

inline std::string ANC_MEM::rand_string(int length) {
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

  std::string result;
  result.reserve(length);
  for (int i = 0; i < length; ++i) {
    result += alphanum[dis(gen)];
  }
  return result;
}

inline std::string ANC_MEM::gen_name() {
  if (cur_name.empty()) {
    cur_name = "a_" + rand_string(3);
    last_idx[cur_name] = -1;
  }

  // Check if current name has reached limit
  if (last_idx[cur_name] >= 99) {
    cur_name = "a_" + rand_string(3);
    last_idx[cur_name] = -1;
  }

  return cur_name;
}

inline qbit ANC_MEM::generate_ancilla(const std::string &prefix) {
  // TODO: take into account the prefix (ignored at this point)
  std::string name = gen_name();
  unsigned int idx = ++last_idx[name];
  qbit ancilla(name, idx);
  mem_.insert(ancilla);
  return ancilla;
}

} // namespace tools
} // namespace tools_v1

#endif
