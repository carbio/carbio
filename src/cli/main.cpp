#include "fingerprint/fingerprint_sensor.h"
#include "main_menu.h"

int main()
{
  spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
  spdlog::set_level(spdlog::level::info);
  carbio::fingerprint_sensor s;
  if (!s.connect("/dev/ttyAMA0"))
    return 1;
  main_menu(s);
}