#include <hubbard/layout.hpp>

#include <stdexcept>
#include <utility>

namespace hubbard {

Layout::Layout(ModelParams params)
    : params_(std::move(params)), config_(params_.L, params_.t, params_.U),
      index_to_coord_(config_.decoding_vector().size()) {
  for (const auto &entry : config_.decoding_vector()) {
    int idx = entry.first;
    if (idx < 0 || idx >= static_cast<int>(index_to_coord_.size())) {
      continue;
    }
    index_to_coord_[static_cast<std::size_t>(idx)] = entry.second;
    coord_to_index_[entry.second] = idx;
  }
}

const ModelParams &Layout::params() const { return params_; }

const square_hubbard_config &Layout::config() const { return config_; }

square_hubbard_config &Layout::config() { return config_; }

int Layout::num_data_qubits() const { return params_.num_fermions(); }

std::vector<Layout::qbit> Layout::data_register(const std::string &name) const {
  std::vector<qbit> data(static_cast<std::size_t>(num_data_qubits()));
  for (int i = 0; i < num_data_qubits(); ++i) {
    data[static_cast<std::size_t>(i)] = qbit(name, i);
  }
  return data;
}

std::pair<int, int> Layout::n_to_nx_ny(int n) const {
  if (n < 0 || n >= static_cast<int>(index_to_coord_.size())) {
    throw std::out_of_range("n_to_nx_ny: index out of range");
  }
  return index_to_coord_[static_cast<std::size_t>(n)];
}

int Layout::nx_ny_to_n(int nx, int ny) const {
  auto it = coord_to_index_.find({nx, ny});
  if (it == coord_to_index_.end()) {
    throw std::out_of_range("nx_ny_to_n: coordinate not found");
  }
  return it->second;
}

} // namespace hubbard
