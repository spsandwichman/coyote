#include <stdint.h>
extern uintptr_t x;
uintptr_t x = 0ULL;
uintptr_t set(uintptr_t _mng_foo1, uintptr_t _mng_y2) {
    uintptr_t _jkl_retv;
    *(int8_t*)(_mng_foo1) = _mng_y2;
    _jkl_retv = _mng_y2;
    goto _jkl_epilogue;
    __twr_l1:;
    _jkl_epilogue:;
    return _jkl_retv;
}
