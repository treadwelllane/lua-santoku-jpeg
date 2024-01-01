local compat = require("santoku.compat")

local env = {

  name = "santoku-jpeg",
  version = "0.0.15-1",
  variable_prefix = "TK_JPEG",
  license = "MIT",
  public = true,

  dependencies = {
    "lua >= 5.1",
    "santoku >= 0.0.152-1",
    "santoku-fs >= 0.0.7-1"
  },

  test = {
    dependencies = {
      "santoku-test >= 0.0.4-1",
      "luacov >= scm-1",
    },
    wasm = {
      ldflags = "--bind",
      dependencies = {
        "santoku-web >= 0.0.85-1"
      }
    }
  },

}

env.homepage = "https://github.com/treadwelllane/lua-" .. env.name
env.tarball = env.name .. "-" .. env.version .. ".tar.gz"
env.download = env.homepage .. "/releases/download/" .. env.version .. "/" .. env.tarball

return {
  type = "lib",
  env = env,
}
