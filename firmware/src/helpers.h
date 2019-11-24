template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

static const int32_t pow10table[] PROGMEM = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

inline int32_t pow10(uint8_t factor) { // from 0 to 9 inclusivly
  return pgm_read_dword_near(pow10table + factor);
}
