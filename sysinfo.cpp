#include "libsysinfo.hpp"
#include <string>
#include <sstream>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#else
#include <climits>
#include <unistd.h>
#include <sys/types.h>
#endif
#if (defined(__APPLE__) && defined(__MACH__))
#include <sys/proc_info.h>
#include <libproc.h>
#endif
#include <cstdlib>
#if (defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__))
#include <sys/sysctl.h>
#if !defined(__FreeBSD__) && !defined(__NetBSD__)
#include <alloca.h>
#endif
#endif
#if defined(__OpenBSD__)
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstddef>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <unistd.h>
#include <kvm.h>
#endif

static std::string get_executable_path() {
  std::string path;
  #if defined(_WIN32)
  wchar_t buffer[MAX_PATH];
  if (GetModuleFileNameW(nullptr, buffer, sizeof(buffer)) != 0) {
    wchar_t exe[MAX_PATH];
    if (_wfullpath(exe, buffer, MAX_PATH)) {
      path = narrow(exe);
    }
  }
  #elif (defined(__APPLE__) && defined(__MACH__))
  char exe[PROC_PIDPATHINFO_MAXSIZE];
  if (proc_pidpath(getpid(), exe, sizeof(exe)) > 0) {
    char buffer[PATH_MAX];
    if (realpath(exe, buffer)) {
      path = buffer;
    }
  }
  #elif (defined(__linux__) && !defined(__ANDROID__))
  char exe[PATH_MAX];
  if (realpath("/proc/self/exe", exe)) {
    path = exe;
  }
  #elif defined(__FreeBSD__) || defined(__DragonFly__)
  int mib[4]; 
  size_t len = 0;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = -1;
  if (sysctl(mib, 4, nullptr, &len, nullptr, 0) == 0) {
    string strbuff;
    strbuff.resize(len, '\0');
    char *exe = strbuff.data();
    if (sysctl(mib, 4, exe, &len, nullptr, 0) == 0) {
      char buffer[PATH_MAX];
      if (realpath(exe, buffer)) {
        path = buffer;
      }
    }
  }
  #elif defined(__NetBSD__)
  int mib[4]; 
  size_t len = 0;
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC_ARGS;
  mib[2] = -1;
  mib[3] = KERN_PROC_PATHNAME;
  if (sysctl(mib, 4, nullptr, &len, nullptr, 0) == 0) {
    string strbuff;
    strbuff.resize(len, '\0');
    char *exe = strbuff.data();
    if (sysctl(mib, 4, exe, &len, nullptr, 0) == 0) {
      char buffer[PATH_MAX];
      if (realpath(exe, buffer)) {
        path = buffer;
      }
    }
  }
  #elif defined(__OpenBSD__)
  auto is_exe = [](std::string exe) {
    int cntp = 0;
    std::string res;
    kvm_t *kd = nullptr;
    kinfo_file *kif = nullptr;
    bool error = false;
    kd = kvm_openfiles(nullptr, nullptr, nullptr, KVM_NO_FILES, nullptr);
    if (!kd) return res;
    if ((kif = kvm_getfiles(kd, KERN_FILE_BYPID, getpid(), sizeof(struct kinfo_file), &cntp))) {
      for (int i = 0; i < cntp && kif[i].fd_fd < 0; i++) {
        if (kif[i].fd_fd == KERN_FILE_TEXT) {
          struct stat st;
          fallback:
          char buffer[PATH_MAX];
          if (!stat(exe.c_str(), &st) && (st.st_mode & S_IXUSR) &&
            (st.st_mode & S_IFREG) && realpath(exe.c_str(), buffer) &&
            st.st_dev == (dev_t)kif[i].va_fsid && st.st_ino == (ino_t)kif[i].va_fileid) {
            res = buffer;
          }
          if (res.empty() && !error) {
            error = true;
            std::size_t last_slash_pos = exe.find_last_of("/");
            if (last_slash_pos != std::string::npos) {
              exe = exe.substr(0, last_slash_pos + 1) + kif[i].p_comm;
              goto fallback;
            }
          }
          break;
        }
      }
    }
    kvm_close(kd);
    return res;
  };
  auto cppstr_getenv = [](std::string name) {
    const char *cresult = getenv(name.c_str());
    std::string result = cresult ? cresult : "";
    return result;
  };
  int cntp = 0;
  kvm_t *kd = nullptr;
  kinfo_proc *proc_info = nullptr;
  std::vector<std::string> buffer;
  bool error = false, retried = false;
  kd = kvm_openfiles(nullptr, nullptr, nullptr, KVM_NO_FILES, nullptr);
  if (!kd) {
    path.clear();
    return path;
  }
  if ((proc_info = kvm_getprocs(kd, KERN_PROC_PID, getpid(), sizeof(struct kinfo_proc), &cntp))) {
    char **cmd = kvm_getargv(kd, proc_info, 0);
    if (cmd) {
      for (int i = 0; cmd[i]; i++) {
        buffer.push_back(cmd[i]);
      }
    }
  }
  kvm_close(kd);
  if (!buffer.empty()) {
    std::string argv0;
    if (!buffer[0].empty()) {
      fallback:
      std::size_t slash_pos = buffer[0].find('/');
      std::size_t colon_pos = buffer[0].find(':');
      if (slash_pos == 0) {
        argv0 = buffer[0];
        path = is_exe(argv0);
      } else if (slash_pos == std::string::npos || slash_pos > colon_pos) { 
        std::string penv = cppstr_getenv("PATH");
        if (!penv.empty()) {
          retry:
          std::string tmp;
          std::stringstream sstr(penv);
          while (std::getline(sstr, tmp, ':')) {
            argv0 = tmp + "/" + buffer[0];
            path = is_exe(argv0);
            if (!path.empty()) break;
            if (slash_pos > colon_pos) {
              argv0 = tmp + "/" + buffer[0].substr(0, colon_pos);
              path = is_exe(argv0);
              if (!path.empty()) break;
            }
          }
        }
        if (path.empty() && !retried) {
          retried = true;
          penv = "/usr/bin:/bin:/usr/sbin:/sbin:/usr/X11R6/bin:/usr/local/bin:/usr/local/sbin";
          std::string home = cppstr_getenv("HOME");
          if (!home.empty()) {
            penv = home + "/bin:" + penv;
          }
          goto retry;
        }
      }
      if (path.empty() && slash_pos > 0) {
        std::string pwd = cppstr_getenv("PWD");
        if (!pwd.empty()) {
          argv0 = pwd + "/" + buffer[0];
          path = is_exe(argv0);
        }
        if (path.empty()) {
          char cwd[PATH_MAX];
          if (getcwd(cwd, PATH_MAX)) {
            argv0 = std::string(cwd) + "/" + buffer[0];
            path = is_exe(argv0);
          }
        }
      }
    }
    if (path.empty() && !error) {
      error = true;
      buffer.clear();
      std::string underscore = cppstr_getenv("_");
      if (!underscore.empty()) {
        buffer.push_back(underscore);
        goto fallback;
      }
    }
  }
  if (!path.empty()) {
    errno = 0;
  }
  #elif defined(__sun)
  char exe[PATH_MAX];
  if (realpath("/proc/self/path/a.out", exe)) {
    path = exe;
  }
  #endif
  return path;
}

static std::string filename_path(std::string fname) {
  #if defined(_WIN32)
  std::size_t fp = fname.find_last_of("\\/");
  #else
  std::size_t fp = fname.find_last_of("/");
  #endif
  if (fp == std::string::npos) return fname;
  return fname.substr(0, fp + 1);
}

static std::string string_replace_all(std::string str, std::string substr, std::string newstr) {
  std::size_t pos = 0;
  const std::size_t sublen = substr.length(), newlen = newstr.length();
  while ((pos = str.find(substr, pos)) != std::string::npos) {
    str.replace(pos, sublen, newstr);
    pos += newlen;
  }
  return str;
}

int main() {
  std::stringstream text; text << 
  "OS DEVICE NAME: " << os_device_name() << "\n" <<
  "OS PRODUCT NAME: " << os_product_name() << "\n" <<
  "OS KERNEL NAME: " << os_kernel_name() << "\n" <<
  "OS KERNEL RELEASE: " << os_kernel_release() << "\n" <<
  "OS ARCHITECTURE: " << os_architecture() << "\n" <<
  "CPU PROCESSOR: " << cpu_processor() << "\n" <<
  "CPU VENDOR: " << cpu_vendor() << "\n" <<
  "CPU CORE COUNT: " << cpu_core_count() << "\n" <<
  "CPU PROCESSOR COUNT: " << cpu_processor_count() << "\n" <<
  "RANDOM-ACCESS MEMORY TOTAL: " << memory_totalram(true) << "\n" <<
  "RANDOM-ACCESS MEMORY USED: " << memory_usedram(true) << "\n" <<
  "RANDOM-ACCESS MEMORY FREE: " << memory_freeram(true) << "\n" <<
  "SWAP MEMORY TOTAL: " << memory_totalswap(true) << "\n" <<
  "SWAP MEMORY USED: " << memory_usedswap(true) << "\n" <<
  "SWAP MEMORY FREE: " << memory_freeswap(true) << "\n" <<
  "GPU MANUFACTURER: " << gpu_manufacturer() << "\n" <<
  "GPU RENDERER: " << gpu_renderer() << "\n" <<
  "GPU MEMORY: " << memory_totalvram(true);
  #if defined(_WIN32)
  auto widen = [](std::string str) {
    if (str.empty()) return L"";
    std::size_t wchar_count = str.size() + 1;
    std::vector<wchar_t> buf(wchar_count);
    return std::wstring{ buf.data(), (std::size_t)MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buf.data(), (int)wchar_count) };
  };
  std::wstring dname = widen(filename_path(get_executable_path())); 
  SetCurrentDirectoryW(dname.c_str());
  SetEnvironmentVariableW(L"IMGUI_DIALOG_WIDTH", L"1024");
  SetEnvironmentVariableW(L"IMGUI_FONT_SIZE", L"24");
  if (system(nullptr) && get_executable_path() != filename_path(get_executable_path()) + "filedialogs.exe") {
    system((std::string("\"") + filename_path(get_executable_path()) + std::string("filedialogs.exe\" --show-message \"") + 
    string_replace_all(text.str(), "\"", "\\\"") + 
    std::string("\" > NUL")).c_str());
  }
  #else
  setenv("IMGUI_DIALOG_WIDTH", "1024", 1);
  setenv("IMGUI_FONT_SIZE", "24", 1);
  chdir(filename_path(get_executable_path()).c_str());
  if (system(nullptr) && get_executable_path() != filename_path(get_executable_path()) + "filedialogs") {
    system((std::string("\"") + filename_path(get_executable_path()) + std::string("filedialogs\" --show-message \"") + 
    string_replace_all(text.str(), "\"", "\\\"") + 
    std::string("\" > /dev/null")).c_str());
  }
  #endif
}