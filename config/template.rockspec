package = "<% return os.getenv('NAME') %>"
version = "<% return os.getenv('VERSION') %>"
rockspec_format = "3.0"

source = {
  url = "git+ssh://<% return os.getenv('GIT_URL') %>",
  tag = "<% return os.getenv('VERSION') %>"
}

description = {
  homepage = "<% return os.getenv('HOMEPAGE') %>",
  license = "<% return os.getenv('LICENSE') %>"
}

dependencies = {
  "lua >= 5.1",
}

test_dependencies = {

  "santoku >= 0.0.87-1",

  <% template:push(os.getenv("EMSCRIPTEN") == "1") %>
  "santoku-web >= 0.0.77-1",
  <% template:pop() %>

  -- TODO: Should this be pulled in by santoku?
  -- It's an optional dependency that for our
  -- purposes here is needed. The alernative is
  -- to maintin luafilesystem as a separate dep
  -- in this array, which isn't so bad.
  "luafilesystem >= 1.8.0-1",

  -- TODO: santoku cli should be a
  -- globally-installed dev dependency checked
  -- for via make
  --
  -- "santoku-cli >= 0.0.22-1",

  "luassert >= 1.9.0-1",

  -- TODO: temporarily using manually installed
  -- broma0/luacov while PR pending:
  --
  -- https://github.com/lunarmodules/luacov/pull/102
  "luacov >= 0.15.0",

  -- TODO: luacheck should also be a
  -- globally-installed dev dependency checked
  -- for via make
  -- "luacheck >= 1.1.0-1",

}

build = {
  type = "make",
  install_target = "luarocks-install",
  build_variables = {
    CFLAGS = "$(CFLAGS)",
    LDFLAGS = "$(LDFLAGS)",
    LIBFLAG = "$(LIBFLAG)",
    LUA_INCDIR = "$(LUA_INCDIR)",
    LUA_LIBDIR = "$(LUA_LIBDIR)",
  },
  install_variables = {
    INST_LIBDIR = "$(LIBDIR)",
    INST_LUADIR = "$(LUADIR)",
  },
}

test = {
  type = "command",
  command = "make luarocks-test"
}
