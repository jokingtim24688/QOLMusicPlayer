#include "update/updater.h"

#include <bcrypt.h>
#include <shellapi.h>
#include <winhttp.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

#include "app_info.h"
#include "util/win.h"

static constexpr wchar_t kRepo[] = L"jokingtim24688/QOLMusicPlayer";
static constexpr wchar_t kAsset[] = L"QOLOverlay-Setup.exe";
static constexpr auto kInterval = std::chrono::hours(6);
static constexpr auto kFirstCheckDelay = std::chrono::seconds(20);  // don't compete with start-up

bool IsNewerVersion(const std::string& candidate, const std::string& current) {
  auto parse = [](const std::string& v, long out[3]) {
    out[0] = out[1] = out[2] = 0;
    const char* p = v.c_str();
    if (*p == 'v' || *p == 'V') ++p;
    for (int i = 0; i < 3 && *p; ++i) {
      char* end = nullptr;
      out[i] = strtol(p, &end, 10);
      if (end == p || *end != '.') break;
      p = end + 1;
    }
  };
  long a[3], b[3];
  parse(candidate, a);
  parse(current, b);
  for (int i = 0; i < 3; ++i)
    if (a[i] != b[i]) return a[i] > b[i];
  return false;
}

// HTTPS GET into memory (body) or a file (path). Follows GitHub's redirect to its download CDN.
static bool HttpGet(const std::wstring& url, std::string* body, const std::wstring* path, std::string* error) {
  URL_COMPONENTS uc{sizeof(uc)};
  wchar_t host[256], urlPath[2048];
  uc.lpszHostName = host;
  uc.dwHostNameLength = 256;
  uc.lpszUrlPath = urlPath;
  uc.dwUrlPathLength = 2048;
  if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) {
    *error = "bad URL";
    return false;
  }
  const std::wstring agent = L"QOLOverlay-Updater/" QOL_VERSION_W;
  HINTERNET session = WinHttpOpen(agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                  WINHTTP_NO_PROXY_BYPASS, 0);
  HINTERNET connect = session ? WinHttpConnect(session, host, uc.nPort, 0) : nullptr;
  HINTERNET request = connect ? WinHttpOpenRequest(connect, L"GET", urlPath, nullptr, WINHTTP_NO_REFERER,
                                                   WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)
                              : nullptr;
  bool ok = false;
  DWORD status = 0, size = sizeof(status);
  if (request && WinHttpSendRequest(request, L"Accept: application/vnd.github+json\r\n", static_cast<DWORD>(-1L),
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
      WinHttpReceiveResponse(request, nullptr) &&
      WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                          &status, &size, WINHTTP_NO_HEADER_INDEX) &&
      status == 200) {
    std::ofstream file;
    if (path) file.open(std::filesystem::path(*path), std::ios::binary | std::ios::trunc);
    std::vector<char> buf(64 * 1024);
    DWORD read = 0;
    ok = !path || file.is_open();
    while (ok && WinHttpReadData(request, buf.data(), static_cast<DWORD>(buf.size()), &read) && read > 0) {
      if (path) file.write(buf.data(), read);
      else body->append(buf.data(), read);
    }
    if (path) ok = ok && file.good();
  }
  if (!ok) *error = status ? "server answered " + std::to_string(status) : "no connection to GitHub";
  if (request) WinHttpCloseHandle(request);
  if (connect) WinHttpCloseHandle(connect);
  if (session) WinHttpCloseHandle(session);
  return ok;
}

static std::string Sha256Hex(const std::wstring& path) {
  BCRYPT_ALG_HANDLE alg = nullptr;
  BCRYPT_HASH_HANDLE hash = nullptr;
  std::string hex;
  if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return hex;
  if (BCryptCreateHash(alg, &hash, nullptr, 0, nullptr, 0, 0) == 0) {
    std::ifstream f(std::filesystem::path(path), std::ios::binary);
    std::vector<char> buf(64 * 1024);
    while (f.read(buf.data(), static_cast<std::streamsize>(buf.size())) || f.gcount() > 0)
      BCryptHashData(hash, reinterpret_cast<PUCHAR>(buf.data()), static_cast<ULONG>(f.gcount()), 0);
    UCHAR digest[32];
    if (BCryptFinishHash(hash, digest, sizeof(digest), 0) == 0) {
      char h[3];
      for (UCHAR b : digest) {
        snprintf(h, sizeof(h), "%02x", b);
        hex += h;
      }
    }
    BCryptDestroyHash(hash);
  }
  BCryptCloseAlgorithmProvider(alg, 0);
  return hex;
}

// Pulls "tag_name": "v0.3.1" out of the release JSON without a JSON library.
static std::string TagName(const std::string& json) {
  size_t k = json.find("\"tag_name\"");
  if (k == std::string::npos) return {};
  size_t a = json.find('"', json.find(':', k) + 1);
  size_t b = a == std::string::npos ? a : json.find('"', a + 1);
  return b == std::string::npos ? std::string() : json.substr(a + 1, b - a - 1);
}

void Updater::Start(HWND notify) {
  notify_ = notify;
  running_ = true;
  thread_ = std::thread([this] { Worker(); });
}

void Updater::Stop() {
  {
    std::lock_guard lock(mu_);
    running_ = false;
  }
  cv_.notify_all();
  if (thread_.joinable()) thread_.join();
}

void Updater::CheckNow() {
  std::lock_guard lock(mu_);
  checkRequested_ = true;
  cv_.notify_all();
}

UpdateStatus Updater::Status() {
  std::lock_guard lock(mu_);
  return status_;
}

void Updater::Worker() {
  auto wait = kFirstCheckDelay;
  for (;;) {
    {
      std::unique_lock lock(mu_);
      cv_.wait_for(lock, wait, [this] { return !running_ || checkRequested_; });
      if (!running_) return;
      checkRequested_ = false;
      if (status_.state == UpdateState::Ready) {  // already have a verified installer waiting
        wait = kInterval;
        continue;
      }
      status_.state = UpdateState::Checking;
      status_.error.clear();
    }
    CheckAndDownload();
    wait = kInterval;
  }
}

bool Updater::CheckAndDownload() {
  auto fail = [this](const std::string& why) {
    std::lock_guard lock(mu_);
    status_.state = UpdateState::Failed;
    status_.error = why;
    return false;
  };

  std::string json, error;
  const std::wstring api = std::wstring(L"https://api.github.com/repos/") + kRepo + L"/releases/latest";
  if (!HttpGet(api, &json, nullptr, &error)) return fail(error);
  const std::string tag = TagName(json);
  if (tag.empty()) return fail("no release found");
  const std::string latest = tag[0] == 'v' ? tag.substr(1) : tag;
  if (!IsNewerVersion(latest, QOL_VERSION)) {
    std::lock_guard lock(mu_);
    status_.state = UpdateState::UpToDate;
    status_.latest = latest;
    return true;
  }
  {
    std::lock_guard lock(mu_);
    status_.state = UpdateState::Downloading;
    status_.latest = latest;
  }

  const std::wstring base =
      std::wstring(L"https://github.com/") + kRepo + L"/releases/download/" + Utf8ToWide(tag) + L"/" + kAsset;
  wchar_t tmp[MAX_PATH];
  GetTempPathW(MAX_PATH, tmp);
  const std::wstring file = std::wstring(tmp) + L"QOLOverlay-Setup-" + Utf8ToWide(latest) + L".exe";
  std::string expected;
  if (!HttpGet(base + L".sha256", &expected, nullptr, &error)) return fail("checksum: " + error);
  if (!HttpGet(base, nullptr, &file, &error)) return fail("download: " + error);

  // The .sha256 file is "<64 hex chars>  QOLOverlay-Setup.exe".
  expected = expected.substr(0, 64);
  for (char& c : expected) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
  if (expected.size() != 64 || Sha256Hex(file) != expected) {
    DeleteFileW(file.c_str());
    return fail("download didn't match its checksum");
  }
  {
    std::lock_guard lock(mu_);
    status_.state = UpdateState::Ready;
    installer_ = file;
  }
  PostMessageW(notify_, kReadyMsg, 0, 0);
  return true;
}

bool Updater::Install() {
  std::wstring file;
  {
    std::lock_guard lock(mu_);
    if (status_.state != UpdateState::Ready) return false;
    file = installer_;
  }
  // /update=1 makes the installer relaunch the overlay when it's done (see installer/QOLOverlay.iss).
  SHELLEXECUTEINFOW sei{sizeof(sei)};
  sei.fMask = SEE_MASK_NOASYNC;
  sei.lpVerb = L"open";
  sei.lpFile = file.c_str();
  sei.lpParameters = L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /update=1";
  sei.nShow = SW_HIDE;
  return ShellExecuteExW(&sei) != FALSE;
}
