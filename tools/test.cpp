#include "grid_synth/gmp_functions.hpp"
#include <gmpxx.h>
using namespace std;


int main() {
    cin.tie(0)->sync_with_stdio(0);

    vector<mpf_class> v;

    using namespace staq::grid_synth;
    
    int prec = 17;
    DEFAULT_GMP_PREC = 4 * prec + 19;
    mpf_set_default_prec(log(10) / log(2) * DEFAULT_GMP_PREC);

    v.emplace_back(exp(mpf_class("-100")));

    for (auto n : v) {
        mp_exp_t e;
        string s = n.get_str(e);
        cout << s << ' ' << e << endl;
    }
    
    

    return 0;
}
