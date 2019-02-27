#if defined(_WIN32)
extern "C" {
  _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

extern "C"
{
  __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif // WIN32