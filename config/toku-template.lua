local os = os

local _ENV = {}

name = "santoku-jpeg"
version = "0.0.10-1"
variable_prefix = "TK_JPEG"

emscripten = os.getenv("EMSCRIPTEN") == "1"

license = "MIT"

dependencies = {
  "lua >= 5.1",
}

test_dependencies = {
  "santoku >= 0.0.87-1",
  "luafilesystem >= 1.8.0-1",
  "luassert >= 1.9.0-1",
  "luacov >= 0.15.0",
}

if emscripten then
  test_dependencies[#test_dependencies + 1] =
    "santoku-web >= 0.0.78-1"
end

homepage = "https://github.com/treadwelllane/lua-" .. name
tarball = name .. "-" .. version .. ".tar.gz"
download = homepage .. "/releases/download/" .. version .. "/" .. tarball

return { env = _ENV }
