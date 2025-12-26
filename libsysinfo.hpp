/*

 MIT License
 
 Copyright © 2023 Samuel Venable
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 
*/

#pragma once
#ifdef _WIN32
#define EXPORTED_FUNCTION extern "C" __declspec(dllexport)
#else
#define EXPORTED_FUNCTION extern "C" __attribute__((visibility("default")))
#endif
EXPORTED_FUNCTION const char *os_kernel_name();
EXPORTED_FUNCTION const char *os_device_name();
EXPORTED_FUNCTION const char *os_kernel_release();
EXPORTED_FUNCTION const char *os_product_name();
EXPORTED_FUNCTION const char *os_kernel_version();
EXPORTED_FUNCTION const char *os_architecture();
EXPORTED_FUNCTION const char *os_is_virtual();
EXPORTED_FUNCTION const char *memory_totalram(double human_readable);
EXPORTED_FUNCTION const char *memory_freeram(double human_readable);
EXPORTED_FUNCTION const char *memory_usedram(double human_readable);
EXPORTED_FUNCTION const char *memory_totalswap(double human_readable);
EXPORTED_FUNCTION const char *memory_freeswap(double human_readable);
EXPORTED_FUNCTION const char *memory_usedswap(double human_readable);
EXPORTED_FUNCTION const char *memory_totalvram(double human_readable);
EXPORTED_FUNCTION const char *gpu_manufacturer();
EXPORTED_FUNCTION const char *gpu_renderer();
EXPORTED_FUNCTION const char *cpu_vendor();
EXPORTED_FUNCTION const char *cpu_processor();
EXPORTED_FUNCTION const char *cpu_processor_count();
EXPORTED_FUNCTION const char *cpu_core_count();
