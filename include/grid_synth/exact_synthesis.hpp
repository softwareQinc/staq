#ifndef EXACT_SYNTHESIS_HPP
#define EXACT_SYNTHESIS_HPP

#include "matrix.hpp"
#include "types.hpp"

namespace staq{
namespace grid_synth {

inline str_t synthesize(const DOmegaMatrix& D,
                        const domega_matrix_table& s3_table) {
    using namespace std;

    int_t s = D.sde_u_sq();
    int_t sold = D.sde_u_sq();
    DOmegaMatrix running_D = D;
    str_t op_str = "";

    while (s > 3) {
        DOmegaMatrix op = H;
        DOmegaMatrix temp_D = op * running_D;
        str_t temp_str = "H";
        if (temp_D.sde_u_sq() == s - 1) {
            op_str += temp_str;
            running_D = temp_D;
            s = temp_D.sde_u_sq();
            continue;
        }

        for (int k = 1; k < 4; k++) {
            op = op * (T.dagger());
            temp_D = op * running_D;
            temp_str = "T" + temp_str;
            if (temp_D.sde_u_sq() == s - 1) {
                op_str += temp_str;
                running_D = temp_D;
                s = temp_D.sde_u_sq();
                break;
            }
        }
        if (sold == s) {
            cout << "Value of s not changed" << endl;
            exit(EXIT_FAILURE);
        }
        sold = sold - 1;
    }
    str_t rem = s3_table.at(running_D);
    return op_str += rem;
}

} // namespace grid_synth
} // namespace staq
  
#endif // EXACT_SYNTHESIS_HPP
