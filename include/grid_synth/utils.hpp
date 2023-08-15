#ifndef UTILS_H
#define UTILS_H

template <typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

#endif // UTILS_H
