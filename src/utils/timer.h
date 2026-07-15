#pragma once

namespace Utils {
class Timer {
public:
  static void Update();
  static float GetDeltaTime();
  static int GetFPS();
  static void SleepExact(int ms);

private:
  inline static float deltaTime = 0.0f;
  inline static int currentFPS = 0;
};
} // namespace Utils
