#include "grid_synth/gmp_functions.hpp"
#include <gmpxx.h>
using namespace std;


int main() {
    cin.tie(0)->sync_with_stdio(0);

    vector<mpf_class> v;

    using namespace staq::grid_synth;
    v.emplace_back(pow(mpf_class("1.100000001"),mpz_class("10000000")));
    v.emplace_back(pow(pow(mpf_class("1.100000001"),mpz_class("10000")),mpz_class("1000")));

    v.emplace_back(exp(mpf_class("-1")));
    v.emplace_back(exp(mpf_class("-10")));
    v.emplace_back(exp(mpf_class("-100")));
    v.emplace_back(exp(mpf_class("100")));
    v.emplace_back(exp(mpf_class("-3.456")));
    v.emplace_back(exp(mpf_class("3.456")));


    for (auto n : v) {
        mp_exp_t e;
        string s = n.get_str(e);
        cout << s << ' ' << e << endl;
    }
    
    

    return 0;
}
