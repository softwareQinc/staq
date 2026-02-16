#ifndef SQUARE_HUBBARD_CONFIG_HPP_
#define SQUARE_HUBBARD_CONFIG_HPP_

#include <cmath>
#include <numbers>
#include <tools_v1/tools/staq_builder.hpp>
#include <unordered_map>
#include <vector>

// Hash function for std::pair<int,int>
namespace std {
template <> struct hash<std::pair<int, int>> {
  size_t operator()(const std::pair<int, int> &p) const {
    return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
  }
};
} // namespace std

struct Ferm_Occ_Idx {
  int nx,ny;
  unsigned int sg;
};

class square_hubbard_config {
private:
  unsigned int _L = 10; // in units of a
  double _t = 1.0;
  double _U = 0.0;
  int _Lmin;
  int _Lmax;
  std::unordered_map<std::pair<int, int>, int> _encoding;
  std::vector<std::pair<int, std::pair<int, int>>> _decoding;

public:
  square_hubbard_config(unsigned int L, double t, double U)
      : _L(L), _t(t), _U(U) {
    _decoding.reserve(L * L);
    _Lmax = (static_cast<int>(L)/2);
    _Lmin = (-static_cast<int>(L+1) / 2 + 1);
    assert( _Lmax-_Lmin+1 == _L );
    for (int nx = -static_cast<int>(L+1) / 2 + 1; nx <= static_cast<int>(L) / 2; ++nx) {
      for (int ny = -static_cast<int>(L+1) / 2 + 1; ny <= static_cast<int>(L) / 2; ++ny) {
        int enc = encoding_formula(nx, ny);
        _encoding[std::make_pair(nx, ny)] = enc;
      }
    }
    for (auto &[k, v] : _encoding) {
      _decoding.emplace_back(v, k);
    }
    std::sort(_decoding.begin(), _decoding.end());
  }

  int brillouin_zone_normalize(int coord){
    while (coord > _Lmax){
      coord -= _L;
    }
    while (coord < _Lmin){
      coord += _L;
    }
    assert( _Lmin <= coord && coord <= _Lmax );
    return coord;
  }

  const int Lmin(){ return _Lmin; }
  const int Lmax(){ return _Lmax; }

  Ferm_Occ_Idx occupation_index(const int& i){
    int mu = i/2;
    int sg = i%2;
    if(sg < 0)
      sg += 2;

    return Ferm_Occ_Idx( 
      _decoding[mu].second.first,
      _decoding[mu].second.second,
      sg
    );
  }

  int index_from_occupation(Ferm_Occ_Idx& f){
    assert(f.sg == 0 || f.sg == 1);
    for(int i = 0; i < _decoding.size(); ++i){
      if(_decoding[i].second.first == f.nx && _decoding[i].second.second == f.ny){
        return 2*i+f.sg;
      }
    }
    std::stringstream ss;
    ss << "qubit index not found for (nx,ny) = (" << f.nx << "," << f.ny << ") with L = " << _L;
    throw ss.str();
  }

  unsigned int L() { return _L; }
  double t() { return _t; }
  double U() { return _U; }


  const std::vector<std::pair<int, std::pair<int, int>>>& decoding_vector() const {
    return _decoding;
  }

  const std::pair<int,std::pair<int,int>> fermion_vals(int& i){
    return _decoding[i];
  }

  const int encode(const int& nx, const int& ny){
    return _encoding[std::make_pair(nx,ny)];
  }

  double e_bare(int nx, int ny) {
    return -2.0 * _t * std::cos(2.0 * std::numbers::pi * nx / double(_L)) -
           2.0 * _t * std::cos(2.0 * std::numbers::pi * ny / double(_L));
  }

  unsigned int encoding_formula(int nx, int ny) {
    // Hybrid formula to handle y=0 case correctly
    // For y != 0: P(x,y) = 2x² + 4sgn(x)sgn(y)xy + 2y² - 2H(x)sgn(y)x - y + 1
    // For y = 0: P(x,0) = 2x² - 2H(x)x + 1
    if (nx == 0 && ny == 0)
      return 0;

    // Calculate sign functions
    int sgn_x = (nx > 0) ? 1 : ((nx < 0) ? -1 : 0);
    int sgn_y = (ny > 0) ? 1 : ((ny < 0) ? -1 : 0);

    // Calculate Heaviside function H(x)
    int H_x = (nx > 0) ? 1 : 0;

    if (ny == 0) {
      // Special case for y=0
      return 2 * nx * nx + 1 - 2 * H_x * nx;
    } else {
      // General case
      return 2 * nx * nx + 1 - 2 * H_x * sgn_y * nx +
             4 * sgn_x * sgn_y * nx * ny + 2 * ny * ny - ny;
    }
  }


};




#endif
