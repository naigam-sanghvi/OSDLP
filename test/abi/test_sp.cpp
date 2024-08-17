#include <osdlp/sp.hpp>

int
main(int argc, char const *argv[])
{
  std::vector<uint8_t>     b  = {0, 1, 2, 3, 4, 5};
  std::vector<int>         b1 = {0, 1, 2, 3, 4, 5};
  volatile osdlp::sp<1024> x0;
  volatile osdlp::sp<1024> x1(b.cbegin(), b.cend());
  return 0;
}
