#include <iostream>
#include "aes.hh"
#include "ope.hh"
#include <ranges>
#include <random>
#include <algorithm>

int main()
{
  std::random_device rd;
  std::mt19937 gen{rd()};
  
  
  ope::OPE o("myKey", 32, 64);
  auto encrypted = o.encrypt(420);
  
  std::cout << encrypted << '\n';
  auto decrypted = o.decrypt(encrypted);
  std::cout << decrypted << '\n';
  
  constexpr auto nums = std::array{1, 10, 100, 101};
  auto nums_encrypted =
    nums
    | std::views::transform([&o](auto &num) { return o.encrypt(num); })
    | std::ranges::to<std::vector>();
  std::cout << "Encrypted in order:\n";
  std::ranges::for_each(
    nums_encrypted,
    [](const auto &num) { std::cout << num << ' '; }
                       );
  std::putchar('\n');
  
  std::ranges::shuffle(nums_encrypted, gen);
  std::cout << "Encrypted shuffled:\n";
  std::ranges::for_each(
    nums_encrypted,
    [](const auto &num) { std::cout << num << ' '; }
                       );
  std::putchar('\n');
  
  std::ranges::sort(nums_encrypted);
  
  std::cout << "Encrypted sorted:\n";
  std::ranges::for_each(
    nums_encrypted,
    [](const auto &num) { std::cout << num << ' '; }
                       );
  std::putchar('\n');
  /*
   * to wydaje się działać :D
   * to teraz musiałbym wzbogacić mojego encryption agenta o to
   */
  
  std::cout << "Hello, World!" << std::endl;
  return 0;
}
