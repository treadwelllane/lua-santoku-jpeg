local env = {

  name = "santoku-jpeg",
  version = "0.0.17-1",
  variable_prefix = "TK_JPEG",
  license = "MIT",
  public = true,

  dependencies = {
    "lua >= 5.1",
    "santoku >= 0.0.193-1",
    "santoku-fs >= 0.0.29-1"
  },

  test = {
    dependencies = {
      "luacov >= 0.15.0-1",
    },
    wasm = {
      ldflags = "--bind",
      dependencies = {
        "santoku-web >= 0.0.87-1"
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
