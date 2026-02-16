#ifndef HUBBARD_LAYOUT_HPP_
#define HUBBARD_LAYOUT_HPP_

#include <square_hubbard_config.hpp>
#include <string>
#include <tools_v1/tools/staq_builder.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

#include <hubbard/model_params.hpp>

namespace hubbard {

class Layout {
public:
  using qbit = tools_v1::tools::qbit;

  explicit Layout(ModelParams params);

  const ModelParams &params() const;
  const square_hubbard_config &config() const;
  square_hubbard_config &config();

  int num_data_qubits() const;
  std::vector<qbit> data_register(const std::string &name) const;

  std::pair<int, int> n_to_nx_ny(int n) const;
  int nx_ny_to_n(int nx, int ny) const;

private:
  ModelParams params_;
  square_hubbard_config config_;
  std::vector<std::pair<int, int>> index_to_coord_;
  std::unordered_map<std::pair<int, int>, int> coord_to_index_;
};

} // namespace hubbard

#endif // HUBBARD_LAYOUT_HPP_
