local compat = require("santoku.compat")

local env = {

  name = "santoku-jpeg",
  version = "0.0.11-1",
  variable_prefix = "TK_JPEG",
  license = "MIT",
  public = true,

  dependencies = {
    "lua >= 5.1",
    "santoku >= 0.0.148-1",
    "santoku-fs >= 0.0.6-1"
  },

  test_dependencies = {
    "santoku-test >= 0.0.4-1",
    "luassert >= 1.9.0-1",
    "luacheck >= 1.1.0-1",
    "luacov >= scm-1",
    compat.unpack(os.getenv("TK_JPEG_WASM") == "1" and {
      "santoku-web >= 0.0.81-1"
    } or {})
  },

}

env.homepage = "https://github.com/treadwelllane/lua-" .. env.name
env.tarball = env.name .. "-" .. env.version .. ".tar.gz"
env.download = env.homepage .. "/releases/download/" .. env.version .. "/" .. env.tarball

return {
  env = env,
}

