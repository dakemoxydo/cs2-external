#pragma once

namespace Utils {
class Timer {
public:
  static void Update();
  static float GetDeltaTime();
  static int GetFPS();
  static void SleepExact(int ms);

private:
  static float deltaTime;
  static int currentFPS;
};
} // namespace Utils
